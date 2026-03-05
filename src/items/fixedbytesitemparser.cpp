/*
 * This file is part of jASTERIX.
 *
 * jASTERIX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * jASTERIX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with jASTERIX.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fixedbytesitemparser.h"

#include <iostream>

#include "logger.h"
#include "string_conv.h"
#include "traced_assert.h"

using namespace std;
using namespace nlohmann;

namespace jASTERIX
{
FixedBytesItemParser::FixedBytesItemParser(const nlohmann::json& item_definition, const std::string& long_name_prefix)
    : ItemParserBase(item_definition, long_name_prefix)
{
    traced_assert(type_ == "fixed_bytes");

    if (!item_definition.contains("length"))
        throw runtime_error("fixed bytes item '" + name_ + "' parsing without length");

    length_ = item_definition.at("length");

    if (!item_definition.contains("data_type"))
        throw runtime_error("fixed bytes item '" + name_ + "' parsing without data type");

    data_type_ = item_definition.at("data_type");

    if (data_type_ == "string")         data_type_enum_ = DataType::String;
    else if (data_type_ == "uint")      data_type_enum_ = DataType::Uint;
    else if (data_type_ == "int")       data_type_enum_ = DataType::Int;
    else if (data_type_ == "bin")       data_type_enum_ = DataType::Bin;
    else
        throw runtime_error("fixed bytes item '" + name_ + "' with unknown data type '" +
                            data_type_ + "'");

    reverse_bits_ =
        (item_definition.contains("reverse_bits") && item_definition.at("reverse_bits") == true);

    reverse_bytes_ =
        (item_definition.contains("reverse_bytes") && item_definition.at("reverse_bytes") == true);

    negative_bit_pos_ = length_ * 8 - 1;

    if (item_definition.contains("lsb"))
    {
        has_lsb_ = true;
        lsb_ = item_definition.at("lsb");
    }
}

size_t FixedBytesItemParser::parseItem(const char* data, size_t index, size_t size,
                                       size_t current_parsed_bytes, size_t total_size,
                                       nlohmann::json& target, bool debug)
{
    if (debug)
        loginf << "parsing fixed bytes item '" << name_ << "' index " << index << " size " << size
               << " current parsed bytes " << current_parsed_bytes << logendl;

    if (index + length_ > total_size)
        throw runtime_error("FixedBytesItemParser '" + name_ + "': would read " +
            to_string(length_) + " bytes at index " + to_string(index) +
            " but total_size is " + to_string(total_size));

    unsigned char tmp{0};
    size_t data_uint{0};
    int data_int{0};

    const char* current_data = &data[index];

    switch (data_type_enum_)
    {
    case DataType::String:
    {
        std::string data_str(reinterpret_cast<char const*>(current_data),
                             length_ - 1);  // -1 to account for end 0

        if (!isASCII(data_str))
            throw runtime_error("fixed bytes item '" + name_ + "' string contains non-ascii chars");

        if (debug)
            loginf << "fixed bytes item '" + name_ + "' parsing index " << index << " length "
                   << length_ << " data type " << data_type_ << " value '" << data_str << "'"
                   << logendl;

        target.emplace(name_, std::move(data_str));

        return length_;
    }
    case DataType::Uint:
    {
        if (length_ > sizeof(size_t))
            throw runtime_error("fixed bytes item '" + name_ + "' length larger than " +
                                to_string(sizeof(size_t)));

        if (reverse_bytes_)
        {
            for (int cnt = length_ - 1; cnt >= 0; --cnt)
            {
                tmp = *reinterpret_cast<const unsigned char*>(&current_data[cnt]);

                if (debug)
                    loginf << "fixed bytes item '" + name_ + "' cnt " << cnt << " byte " << std::hex
                           << static_cast<unsigned int>(tmp) << " reverse bytes false bits "
                           << reverse_bits_ << " data " << data_uint << logendl;

                if (reverse_bits_)
                    reverseBits(tmp);

                data_uint = (data_uint << 8) + tmp;
            }
        }
        else
        {
            for (size_t cnt = 0; cnt < length_; ++cnt)
            {
                tmp = *reinterpret_cast<const unsigned char*>(&current_data[cnt]);

                if (debug)
                    loginf << "fixed bytes item '" + name_ + "' cnt " << cnt << " byte " << std::hex
                           << static_cast<unsigned int>(tmp) << " reverse bytes true bits "
                           << reverse_bits_ << " data " << data_uint << logendl;

                if (reverse_bits_)
                    reverseBits(tmp);

                data_uint = (data_uint << 8) + tmp;
            }
        }

        if (debug)
            loginf << "parsing fixed bytes item '" + name_ + "' index " << index << " length "
                   << length_ << " data type " << data_type_ << " value '" << data_uint << "'"
                   << (has_lsb_ ? " lsb " + to_string(lsb_) : "") << logendl;

        if (has_lsb_)
            target.emplace(name_, lsb_ * data_uint);
        else
            target.emplace(name_, data_uint);

        return length_;
    }
    case DataType::Int:
    {
        if (length_ > sizeof(size_t))
            throw runtime_error("fixed bytes item '" + name_ + "' length larger than " +
                                to_string(sizeof(size_t)));

        if (reverse_bytes_)
        {
            for (int cnt = length_ - 1; cnt >= 0; --cnt)
            {
                tmp = *reinterpret_cast<const unsigned char*>(&current_data[cnt]);

                if (debug)
                    loginf << "fixed bytes item '" + name_ + "' cnt " << cnt << " byte " << std::hex
                           << static_cast<unsigned int>(tmp) << " reverse bytes false bits "
                           << reverse_bits_ << " data " << data_uint << logendl;

                if (reverse_bits_)
                    reverseBits(tmp);

                data_uint = (data_uint << 8) + tmp;
            }
        }
        else
        {
            for (size_t cnt = 0; cnt < length_; ++cnt)
            {
                tmp = *reinterpret_cast<const unsigned char*>(&current_data[cnt]);

                if (debug)
                    loginf << "fixed bytes item '" + name_ + "' cnt " << cnt << " byte " << std::hex
                           << static_cast<unsigned int>(tmp) << " reverse bytes true bits "
                           << reverse_bits_ << " data " << data_uint << logendl;

                if (reverse_bits_)
                    reverseBits(tmp);

                data_uint = (data_uint << 8) + tmp;
            }
        }

        if ((data_uint & (1 << negative_bit_pos_)) != 0)
            data_int = data_uint | ~((1 << negative_bit_pos_) - 1);
        else
            data_int = data_uint;

        if (debug)
            loginf << "parsing fixed bytes item '" + name_ + "' index " << index << " length "
                   << length_ << " data type " << data_type_ << " value '" << data_int << "'"
                   << (has_lsb_ ? " lsb " + to_string(lsb_) : "") << logendl;

        if (has_lsb_)
            target.emplace(name_, lsb_ * data_int);
        else
            target.emplace(name_, data_int);

        return length_;
    }
    case DataType::Bin:
    {
        std::string data_str =
            binary2hex(reinterpret_cast<const unsigned char*>(current_data), length_);

        if (debug)
        {
            loginf << "fixed bytes item '" + name_ + "' parsing index " << index << " length "
                   << length_ << " data type " << data_type_ << " value '" << data_str << "'"
                   << logendl;
        }

        target.emplace(name_, std::move(data_str));

        return length_;
    }
    }

    return length_;  // unreachable, all enum values handled
}

size_t FixedBytesItemParser::encodeItem(const nlohmann::json& source, char* target,
                                        size_t max_size, bool debug)
{
    if (debug)
        loginf << "encoding fixed bytes item '" << name_ << "' length " << length_
               << " data type " << data_type_ << logendl;

    const auto& value = source.at(name_);

    if (data_type_ == "string")
    {
        std::string str_val = value.get<std::string>();
        memset(target, 0, length_);
        memcpy(target, str_val.c_str(), std::min(str_val.size(), length_ - 1));
        return length_;
    }
    else if (data_type_ == "uint")
    {
        size_t data_uint;

        if (has_lsb_)
            data_uint = static_cast<size_t>(llround(value.get<double>() / lsb_));
        else
            data_uint = value.get<size_t>();

        if (reverse_bytes_)
        {
            for (size_t cnt = 0; cnt < length_; ++cnt)
            {
                unsigned char byte = static_cast<unsigned char>((data_uint >> (8 * cnt)) & 0xFF);
                if (reverse_bits_)
                    byte = reverseBits(byte);
                target[cnt] = static_cast<char>(byte);
            }
        }
        else
        {
            for (int cnt = length_ - 1; cnt >= 0; --cnt)
            {
                unsigned char byte = static_cast<unsigned char>(data_uint & 0xFF);
                if (reverse_bits_)
                    byte = reverseBits(byte);
                target[cnt] = static_cast<char>(byte);
                data_uint >>= 8;
            }
        }

        return length_;
    }
    else if (data_type_ == "int")
    {
        long int data_int;

        if (has_lsb_)
            data_int = llround(value.get<double>() / lsb_);
        else
            data_int = value.get<long int>();

        // mask to length_ bytes for two's complement
        size_t data_uint = static_cast<size_t>(data_int) & ((size_t(1) << (length_ * 8)) - 1);

        if (reverse_bytes_)
        {
            for (size_t cnt = 0; cnt < length_; ++cnt)
            {
                unsigned char byte = static_cast<unsigned char>((data_uint >> (8 * cnt)) & 0xFF);
                if (reverse_bits_)
                    byte = reverseBits(byte);
                target[cnt] = static_cast<char>(byte);
            }
        }
        else
        {
            for (int cnt = length_ - 1; cnt >= 0; --cnt)
            {
                unsigned char byte = static_cast<unsigned char>(data_uint & 0xFF);
                if (reverse_bits_)
                    byte = reverseBits(byte);
                target[cnt] = static_cast<char>(byte);
                data_uint >>= 8;
            }
        }

        return length_;
    }
    else if (data_type_ == "bin")
    {
        std::string hex_str = value.get<std::string>();
        hex2bin(hex_str.c_str(), target);
        return length_;
    }
    else
        throw runtime_error("fixed bytes item '" + name_ + "' encoding with unknown data type '" +
                            data_type_ + "'");
}

}  // namespace jASTERIX
