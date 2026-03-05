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

#include "fixedbitsitemparser.h"

#include <algorithm>
#include <boost/dynamic_bitset.hpp>

#include "logger.h"
#include "string_conv.h"
#include "traced_assert.h"

using namespace std;
using namespace nlohmann;

//#define debug_ name_=="Last Measured Mode C Code"

namespace jASTERIX
{
FixedBitsItemParser::FixedBitsItemParser(const nlohmann::json& item_definition, const std::string& long_name_prefix,
                                         unsigned int byte_length)
    : ItemParserBase(item_definition, long_name_prefix), byte_length_(byte_length)
{
    traced_assert(type_ == "fixed_bits");

    if (!item_definition.contains("start_bit"))
        throw runtime_error("fixed byte bitfield item '" + name_ + "' without start bit");

    start_bit_ = item_definition.at("start_bit");

    if (!item_definition.contains("bit_length"))
        throw runtime_error("fixed byte bitfield item '" + name_ + "' without bit length");

    bit_length_ = item_definition.at("bit_length");

    if (bit_length_ == 0)
        throw runtime_error("fixed byte bitfield item '" + name_ + "' with 0 bit length");

    if (start_bit_ + bit_length_ > byte_length * 8)
        throw runtime_error("fixed byte bitfield item '" + name_ + "' wrong length " +
                            to_string(byte_length * 8) + " for bitsize " +
                            to_string(start_bit_ + bit_length_));

    if (item_definition.contains("data_type"))
        data_type_ = item_definition.at("data_type");

    if (data_type_ == "uint")                    data_type_enum_ = DataType::Uint;
    else if (data_type_ == "int")                data_type_enum_ = DataType::Int;
    else if (data_type_ == "digits")             data_type_enum_ = DataType::Digits;
    else if (data_type_ == "icao_characters")    data_type_enum_ = DataType::IcaoCharacters;
    else if (data_type_ == "ascii_characters")   data_type_enum_ = DataType::AsciiCharacters;
    // unknown types will be caught by the constructor's existing validation below

    if (item_definition.contains("lsb"))
    {
        has_lsb_ = true;
        lsb_ = item_definition.at("lsb");
    }

    if (data_type_ == "int")
    {
        //negative_bit_pos_ = start_bit_ + bit_length_ - 1;

        if (byte_length_ == 1)
        {
            neg_bitmask1 = 1 << (bit_length_ - 1);
        }
        else if (byte_length_ <= 4)
        {
            neg_bitmask4 = 1 << (bit_length_ - 1);
        }
        else if (byte_length_ <= 8)
        {
            neg_bitmask8 = 1 << (bit_length_ - 1);
        }
    }

    if (data_type_ == "uint" || data_type_ == "int")
    {
        if (byte_length_ == 1)
        {
            bitmask1 = 1;
            for (unsigned cnt = 0; cnt < bit_length_ - 1; ++cnt)
            {
                bitmask1 <<= 1;
                bitmask1 += 1;
            }
            bitmask1 <<= start_bit_;
        }
        else if (byte_length_ <= 4)
        {
            bitmask4 = 1;
            for (unsigned cnt = 0; cnt < bit_length_ - 1; ++cnt)
            {
                bitmask4 <<= 1;
                bitmask4 += 1;
            }
            bitmask4 <<= start_bit_;
        }
        else if (byte_length_ <= 8)
        {
            bitmask8 = 1;
            for (unsigned cnt = 0; cnt < bit_length_ - 1; ++cnt)
            {
                bitmask8 <<= 1;
                bitmask8 += 1;
            }
            bitmask8 <<= start_bit_;
        }
        else
            throw runtime_error("fixed byte bitfield item '" + name_ + "' with length" +
                                to_string(byte_length_));
    }
    else if (data_type_ == "digits")
    {
        if (!item_definition.contains("num_digits"))
            throw runtime_error("fixed byte bitfield item '" + name_ +
                                "' data type digits without number of digits");

        num_digits_ = item_definition.at("num_digits");

        if (!item_definition.contains("digit_bit_length"))
            throw runtime_error("fixed byte bitfield item '" + name_ +
                                "' data type digits without digit bit length");

        digit_bit_length_ = item_definition.at("digit_bit_length");

        if (start_bit_ + num_digits_ * digit_bit_length_ > byte_length * 8)
            throw runtime_error("fixed byte bitfield item '" + name_ + "' wrong length " +
                                to_string(byte_length * 8) + " for digits bitsize " +
                                to_string(start_bit_ + num_digits_ * digit_bit_length_));
        if (byte_length_ == 1)
        {
            bitmask1 = 1;
            for (unsigned cnt = 0; cnt < digit_bit_length_ - 1; ++cnt)
            {
                bitmask1 <<= 1;
                bitmask1 += 1;
            }
            bitmask1 <<= start_bit_;

            for (unsigned cnt = 0; cnt < num_digits_; ++cnt)
            {
                digits_bitmasks1.push_back(bitmask1);
                bitmask1 <<= digit_bit_length_;
            }

            bitmask1 = 0;
            traced_assert(digits_bitmasks1.size() == num_digits_);
        }
        else if (byte_length_ <= 4)
        {
            bitmask4 = 1;
            for (unsigned cnt = 0; cnt < digit_bit_length_ - 1; ++cnt)
            {
                bitmask4 <<= 1;
                bitmask4 += 1;
            }

            bitmask4 <<= start_bit_;

            for (unsigned cnt = 0; cnt < num_digits_; ++cnt)
            {
                digits_bitmasks4.push_back(bitmask4);
                bitmask4 <<= digit_bit_length_;
            }
            // reverse(digits_bitmasks4.begin(), digits_bitmasks4.end());

            bitmask4 = 0;
            traced_assert(digits_bitmasks4.size() == num_digits_);
        }
        else if (byte_length_ <= 8)
        {
            bitmask8 = 1;
            for (unsigned cnt = 0; cnt < digit_bit_length_ - 1; ++cnt)
            {
                bitmask8 <<= 1;
                bitmask8 += 1;
            }

            bitmask8 <<= start_bit_;

            for (unsigned cnt = 0; cnt < num_digits_; ++cnt)
            {
                digits_bitmasks8.push_back(bitmask8);
                bitmask8 <<= digit_bit_length_;
            }
            // reverse(digits_bitmasks8.begin(), digits_bitmasks8.end());

            bitmask8 = 0;
            traced_assert(digits_bitmasks8.size() == num_digits_);
        }
        else
            throw runtime_error("fixed byte bitfield item '" + name_ + "' with length" +
                                to_string(byte_length_));
    }
    else if (data_type_ == "icao_characters" || data_type_ == "ascii_characters")
    {
        if (!item_definition.contains("num_characters"))
            throw runtime_error("fixed byte bitfield item '" + name_ +
                                "' data type characters without number of characters");

        num_characters_ = item_definition.at("num_characters");

        if (!item_definition.contains("character_bit_length"))
            throw runtime_error("fixed byte bitfield item '" + name_ +
                                "' data type characters without characters bit length");

        character_bit_length_ = item_definition.at("character_bit_length");

        if (start_bit_ + num_digits_ * digit_bit_length_ > byte_length * 8)
            throw runtime_error("fixed byte bitfield item '" + name_ + "' wrong length " +
                                to_string(byte_length * 8) + " for digits bitsize " +
                                to_string(start_bit_ + num_digits_ * digit_bit_length_));

        if (data_type_ == "icao_characters" && character_bit_length_ != 6)
            throw invalid_argument("fixed byte bitfield item '" + name_ +
                                   "' wrong icao character bit length " +
                                   to_string(character_bit_length_));

        if (data_type_ == "ascii_characters" && character_bit_length_ != 8)
            throw invalid_argument("fixed byte bitfield item '" + name_ +
                                   "' wrong ascii character bit length " +
                                   to_string(character_bit_length_));

        if (byte_length_ == 1)
        {
            bitmask1 = 1;
            for (unsigned cnt = 0; cnt < character_bit_length_ - 1; ++cnt)
            {
                bitmask1 <<= 1;
                bitmask1 += 1;
            }
            bitmask1 <<= start_bit_;

            for (unsigned cnt = 0; cnt < num_characters_; ++cnt)
            {
                chars_bitmasks1.push_back(bitmask1);
                bitmask1 <<= character_bit_length_;
            }

            bitmask1 = 0;
            traced_assert(chars_bitmasks1.size() == num_characters_);
        }
        else if (byte_length_ <= 4)
        {
            bitmask4 = 1;
            for (unsigned cnt = 0; cnt < character_bit_length_ - 1; ++cnt)
            {
                bitmask4 <<= 1;
                bitmask4 += 1;
            }

            bitmask4 <<= start_bit_;

            for (unsigned cnt = 0; cnt < num_characters_; ++cnt)
            {
                chars_bitmasks4.push_back(bitmask4);
                bitmask4 <<= character_bit_length_;
            }

            bitmask4 = 0;
            traced_assert(chars_bitmasks4.size() == num_characters_);
        }
        else if (byte_length_ <= 8)
        {
            bitmask8 = 1;
            for (unsigned cnt = 0; cnt < character_bit_length_ - 1; ++cnt)
            {
                bitmask8 <<= 1;
                bitmask8 += 1;
            }

            bitmask8 <<= start_bit_;

            for (unsigned cnt = 0; cnt < num_characters_; ++cnt)
            {
                chars_bitmasks8.push_back(bitmask8);
                bitmask8 <<= character_bit_length_;
            }

            bitmask8 = 0;
            traced_assert(chars_bitmasks8.size() == num_characters_);
        }
        else
            throw runtime_error("fixed byte bitfield item '" + name_ + "' with length" +
                                to_string(byte_length_));
    }
    else
        throw runtime_error("fixed byte bitfield item '" + name_ + "' with unknown data type '" +
                            data_type_ + "'");
}

size_t FixedBitsItemParser::parseItem(const char* data, size_t index, size_t size,
                                      size_t current_parsed_bytes, size_t total_size,
                                      nlohmann::json& target, bool debug)
{
    if (debug)
        loginf << "parsing fixed bits item '" << name_ << "' byte length " << byte_length_
               << " index " << index << " size " << size << " current parsed bytes "
               << current_parsed_bytes << logendl;

    if (index + byte_length_ > total_size)
        throw runtime_error("FixedBitsItemParser '" + name_ + "': would read " +
            to_string(byte_length_) + " bytes at index " + to_string(index) +
            " but total_size is " + to_string(total_size));

    unsigned char tmp1{0};

    if (byte_length_ == 1)
    {
        tmp1 = *reinterpret_cast<const unsigned char*>(&data[index]);

        switch (data_type_enum_)
        {
        case DataType::Uint:
        {
            tmp1 &= bitmask1;
            tmp1 >>= start_bit_;

            if (debug)
                loginf << "parsing fixed bits item '" << name_ << "' with start bit " << start_bit_
                       << " length " << bit_length_ << " value " << (size_t)tmp1 << logendl;

            if (has_lsb_)
                target.emplace(name_, lsb_ * tmp1);
            else
                target.emplace(name_, tmp1);
            break;
        }
        case DataType::Int:
        {
            tmp1 &= bitmask1;
            tmp1 >>= start_bit_;

            if (debug)
                loginf << "parsing fixed bits item '" << name_ << "' with start bit " << start_bit_
                       << " length " << bit_length_ << " value " << (size_t)tmp1 << logendl;

            int data_int;

            if ((tmp1 & neg_bitmask1) != 0)
                data_int = tmp1 | ~((neg_bitmask1) - 1);
            else
                data_int = tmp1;

            if (has_lsb_)
            {
                if (debug)
                    loginf << "parsing fixed bits item '" << name_ << "' with start bit " << start_bit_
                        << " length " << bit_length_ << " value " << (size_t)tmp1 << " parsed "
                        << data_int << logendl;

                target.emplace(name_, lsb_ * data_int);
            }
            else
                target.emplace(name_, data_int);
            break;
        }
        case DataType::Digits:
        {
            size_t digits_tmp{0};
            size_t digit_tmp1{0};

            for (int cnt = num_digits_ - 1; cnt >= 0; --cnt)
            {
                digits_tmp *= 10;
                digit_tmp1 = tmp1 & digits_bitmasks1[cnt];
                digit_tmp1 >>= cnt * character_bit_length_;
                digits_tmp += digit_tmp1;

                if (debug)
                    loginf << "parsing fixed bits item '" << name_ << "' type digits cnt " << cnt
                           << " digits1 tmp " << digits_tmp << " value " << (size_t)tmp1
                           << " bitmask " << digits_bitmasks1[cnt] << logendl;
            }
            target.emplace(name_, digits_tmp);
            break;
        }
        case DataType::IcaoCharacters:
        case DataType::AsciiCharacters:
        {
            string characters_tmp;
            characters_tmp.reserve(num_characters_);
            char char_tmp1;
            bool is_icao = (data_type_enum_ == DataType::IcaoCharacters);

            for (int cnt = num_characters_ - 1; cnt >= 0; --cnt)
            {
                char_tmp1 = tmp1 & chars_bitmasks1[cnt];
                char_tmp1 >>= cnt * character_bit_length_;

                if (is_icao)
                    characters_tmp += getIcaoChar(char_tmp1);
                else
                    characters_tmp += char_tmp1;

                if (debug)
                    loginf << "parsing fixed bits item '" << name_ << "' type " << data_type_
                           << " cnt " << cnt << " characters tmp '" << characters_tmp << "' value "
                           << (size_t)char_tmp1 << " bitmask " << chars_bitmasks1[cnt] << logendl;
            }
            target.emplace(name_, std::move(characters_tmp));
            break;
        }
        }
    }
    else if (byte_length_ <= 4)
    {
        size_t tmp4{0};

        for (size_t cnt = 0; cnt < byte_length_; ++cnt)
        {
            tmp1 = *reinterpret_cast<const unsigned char*>(&data[index + cnt]);
            tmp4 = (tmp4 << 8) + tmp1;

            if (debug)
                loginf << "parsing fixed bits item '" << name_ << "' tmp1 " << (size_t)tmp1
                       << " tmp4 " << (size_t)tmp4 << logendl;
        }

        switch (data_type_enum_)
        {
        case DataType::Uint:
        {
            tmp4 &= bitmask4;
            tmp4 >>= start_bit_;

            if (debug)
                loginf << "parsing fixed bits item '" << name_ << "' with start bit " << start_bit_
                       << " length " << bit_length_ << " value " << (size_t)tmp4;

            if (has_lsb_)
                target.emplace(name_, lsb_ * tmp4);
            else
                target.emplace(name_, tmp4);
            break;
        }
        case DataType::Int:
        {
            tmp4 &= bitmask4;
            tmp4 >>= start_bit_;

//            loginf << "UGA tmp4 " << boost::dynamic_bitset<> (byte_length_*8, tmp4);
//            loginf << "UGA ngbm " << boost::dynamic_bitset<> (byte_length_*8, neg_bitmask4);

            if (debug)
                loginf << "parsing fixed bits item '" << name_ << "' with start bit " << start_bit_
                       << " length " << bit_length_ << " value " << (size_t)tmp4 << " neg bit pos "
                       //<< negative_bit_pos_ << " set " << (tmp4 & neg_bitmask4)
                       << logendl;

            int data_int;

            if ((tmp4 & (neg_bitmask4)) != 0)
            {
                if (debug)
                    loginf << "parsing fixed bits item '" << name_ << "' negative bit "
                           << (size_t)tmp4 << logendl;

                data_int = tmp4 | ~((neg_bitmask4) - 1);
            }
            else
                data_int = tmp4;

            if (has_lsb_)
            {
                if (debug)
                    loginf << "parsing fixed bits item '" << name_ << "' with start bit "
                           << start_bit_ << " length " << bit_length_ << " final value "
                           << lsb_ * data_int << logendl;
                target.emplace(name_, lsb_ * data_int);
            }
            else
                target.emplace(name_, data_int);
            break;
        }
        case DataType::Digits:
        {
            size_t digits_tmp{0};
            size_t digit_tmp;

            for (int cnt = num_digits_ - 1; cnt >= 0; --cnt)
            {
                digits_tmp *= 10;
                digit_tmp = tmp4 & digits_bitmasks4[cnt];
                digit_tmp >>= cnt * digit_bit_length_;
                digits_tmp += digit_tmp;

                if (debug)
                    loginf << "parsing fixed bits item '" << name_ << "' type digits cnt " << cnt
                           << " digits4 tmp " << digits_tmp << " value " << (size_t)tmp4
                           << " bitmask " << digits_bitmasks4[cnt] << logendl;
            }
            target.emplace(name_, digits_tmp);
            break;
        }
        case DataType::IcaoCharacters:
        case DataType::AsciiCharacters:
        {
            string characters_tmp;
            characters_tmp.reserve(num_characters_);
            size_t char_tmp4;
            bool is_icao = (data_type_enum_ == DataType::IcaoCharacters);

            for (int cnt = num_characters_ - 1; cnt >= 0; --cnt)
            {
                char_tmp4 = tmp4 & chars_bitmasks4[cnt];
                char_tmp4 >>= cnt * character_bit_length_;

                if (is_icao)
                    characters_tmp += getIcaoChar(char_tmp4);
                else
                    characters_tmp += static_cast<unsigned char>(char_tmp4);

                if (debug)
                    loginf << "parsing fixed bits item '" << name_ << "' type characters cnt "
                           << cnt << " characters tmp '" << characters_tmp << "' value "
                           << (size_t)char_tmp4 << " bitmask " << chars_bitmasks4[cnt] << logendl;
            }
            target.emplace(name_, std::move(characters_tmp));
            break;
        }
        }
    }
    else if (byte_length_ <= 8)
    {
        size_t tmp8{0};

        for (size_t cnt = 0; cnt < byte_length_; ++cnt)
        {
            tmp1 = *reinterpret_cast<const unsigned char*>(&data[index + cnt]);
            tmp8 = (tmp8 << 8) + tmp1;

            if (debug)
                loginf << "parsing fixed bits item '" << name_ << "' tmp1 " << (size_t)tmp1
                       << " tmp8 " << (size_t)tmp8 << logendl;
        }

        switch (data_type_enum_)
        {
        case DataType::Uint:
        {
            tmp8 &= bitmask8;
            tmp8 >>= start_bit_;

            if (debug)
                loginf << "parsing fixed bits item '" << name_ << "' with start bit " << start_bit_
                       << " length " << bit_length_ << " value " << (size_t)tmp8 << logendl;

            if (has_lsb_)
                target.emplace(name_, lsb_ * tmp8);
            else
                target.emplace(name_, tmp8);
            break;
        }
        case DataType::Int:
        {
            tmp8 &= bitmask8;
            tmp8 >>= start_bit_;

            if (debug)
                loginf << "parsing fixed bits item '" << name_ << "' with start bit " << start_bit_
                       << " length " << bit_length_ << " value " << (size_t)tmp8 << logendl;

            long int data_lint;

            if ((tmp8 & (neg_bitmask8)) != 0)
                data_lint = tmp8 | ~((neg_bitmask8) - 1);
            else
                data_lint = tmp8;

            if (has_lsb_)
                target.emplace(name_, lsb_ * data_lint);
            else
                target.emplace(name_, data_lint);
            break;
        }
        case DataType::Digits:
        {
            size_t digits_tmp{0};

            for (int cnt = num_digits_ - 1; cnt >= 0; --cnt)
            {
                if (debug)
                    loginf << "parsing fixed bits item '" << name_ << "' type digits cnt " << cnt
                           << " digits8 tmp " << digits_tmp << " value " << (size_t)tmp8
                           << " bitmask " << digits_bitmasks8[cnt] << logendl;

                digits_tmp *= 10;
                digits_tmp += tmp8 & digits_bitmasks8[cnt];
            }
            target.emplace(name_, digits_tmp);
            break;
        }
        case DataType::IcaoCharacters:
        case DataType::AsciiCharacters:
        {
            string characters_tmp;
            characters_tmp.reserve(num_characters_);
            size_t char_tmp8;
            bool is_icao = (data_type_enum_ == DataType::IcaoCharacters);

            for (int cnt = num_characters_ - 1; cnt >= 0; --cnt)
            {
                char_tmp8 = tmp8 & chars_bitmasks8[cnt];
                char_tmp8 >>= cnt * character_bit_length_;

                if (is_icao)
                    characters_tmp += getIcaoChar(char_tmp8);
                else
                    characters_tmp += static_cast<unsigned char>(char_tmp8);

                if (debug)
                    loginf << "parsing fixed bits item '" << name_ << "' type characters cnt "
                           << cnt << " characters tmp '" << characters_tmp << "' value "
                           << (size_t)char_tmp8 << " bitmask " << chars_bitmasks8[cnt] << logendl;
            }
            target.emplace(name_, std::move(characters_tmp));
            break;
        }
        }
    }

    return 0;
}

size_t FixedBitsItemParser::encodeItem(const nlohmann::json& source, char* target,
                                       size_t max_size, bool debug)
{
    if (debug)
        loginf << "encoding fixed bits item '" << name_ << "' byte_length " << byte_length_
               << " start_bit " << start_bit_ << " bit_length " << bit_length_ << logendl;

    if (byte_length_ == 1)
    {
        unsigned char& byte_ref = *reinterpret_cast<unsigned char*>(&target[0]);

        if (data_type_ == "uint")
        {
            size_t raw;
            if (has_lsb_)
                raw = static_cast<size_t>(llround(source.at(name_).get<double>() / lsb_));
            else
                raw = source.at(name_).get<size_t>();

            byte_ref |= static_cast<unsigned char>((raw << start_bit_) & bitmask1);
        }
        else if (data_type_ == "int")
        {
            long int raw;
            if (has_lsb_)
                raw = llround(source.at(name_).get<double>() / lsb_);
            else
                raw = source.at(name_).get<long int>();

            unsigned char masked = static_cast<unsigned char>(raw & ((1 << bit_length_) - 1));
            byte_ref |= static_cast<unsigned char>((masked << start_bit_) & bitmask1);
        }
        else if (data_type_ == "digits")
        {
            size_t digits_val = source.at(name_).get<size_t>();

            for (unsigned int cnt = 0; cnt < num_digits_; ++cnt)
            {
                size_t digit = digits_val % 10;
                digits_val /= 10;
                byte_ref |= static_cast<unsigned char>((digit << (cnt * digit_bit_length_ + start_bit_))
                                                        & digits_bitmasks1[cnt]);
            }
        }
        else if (data_type_ == "icao_characters" || data_type_ == "ascii_characters")
        {
            std::string str_val = source.at(name_).get<std::string>();

            for (unsigned int cnt = 0; cnt < num_characters_ && cnt < str_val.size(); ++cnt)
            {
                unsigned char code;
                if (data_type_ == "icao_characters")
                    code = getIcaoCode(str_val[str_val.size() - 1 - cnt]);
                else
                    code = static_cast<unsigned char>(str_val[str_val.size() - 1 - cnt]);

                byte_ref |= static_cast<unsigned char>((code << (cnt * character_bit_length_ + start_bit_))
                                                        & chars_bitmasks1[cnt]);
            }
        }
    }
    else if (byte_length_ <= 4)
    {
        // read existing bytes into tmp4 for OR'ing
        unsigned int tmp4{0};
        for (size_t cnt = 0; cnt < byte_length_; ++cnt)
        {
            tmp4 = (tmp4 << 8) + static_cast<unsigned char>(target[cnt]);
        }

        if (data_type_ == "uint")
        {
            size_t raw;
            if (has_lsb_)
                raw = static_cast<size_t>(llround(source.at(name_).get<double>() / lsb_));
            else
                raw = source.at(name_).get<size_t>();

            tmp4 |= static_cast<unsigned int>((raw << start_bit_) & bitmask4);
        }
        else if (data_type_ == "int")
        {
            long int raw;
            if (has_lsb_)
                raw = llround(source.at(name_).get<double>() / lsb_);
            else
                raw = source.at(name_).get<long int>();

            unsigned int masked = static_cast<unsigned int>(raw & ((1u << bit_length_) - 1));
            tmp4 |= (masked << start_bit_) & bitmask4;
        }
        else if (data_type_ == "digits")
        {
            size_t digits_val = source.at(name_).get<size_t>();

            for (unsigned int cnt = 0; cnt < num_digits_; ++cnt)
            {
                size_t digit = digits_val % 10;
                digits_val /= 10;
                tmp4 |= static_cast<unsigned int>((digit << (cnt * digit_bit_length_ + start_bit_))
                                                   & digits_bitmasks4[cnt]);
            }
        }
        else if (data_type_ == "icao_characters" || data_type_ == "ascii_characters")
        {
            std::string str_val = source.at(name_).get<std::string>();

            for (unsigned int cnt = 0; cnt < num_characters_ && cnt < str_val.size(); ++cnt)
            {
                size_t code;
                if (data_type_ == "icao_characters")
                    code = getIcaoCode(str_val[str_val.size() - 1 - cnt]);
                else
                    code = static_cast<unsigned char>(str_val[str_val.size() - 1 - cnt]);

                tmp4 |= static_cast<unsigned int>((code << (cnt * character_bit_length_ + start_bit_))
                                                   & chars_bitmasks4[cnt]);
            }
        }

        // write back bytes
        for (int cnt = byte_length_ - 1; cnt >= 0; --cnt)
        {
            target[cnt] = static_cast<char>(tmp4 & 0xFF);
            tmp4 >>= 8;
        }
    }
    else if (byte_length_ <= 8)
    {
        // read existing bytes into tmp8 for OR'ing
        size_t tmp8{0};
        for (size_t cnt = 0; cnt < byte_length_; ++cnt)
        {
            tmp8 = (tmp8 << 8) + static_cast<unsigned char>(target[cnt]);
        }

        if (data_type_ == "uint")
        {
            size_t raw;
            if (has_lsb_)
                raw = static_cast<size_t>(llround(source.at(name_).get<double>() / lsb_));
            else
                raw = source.at(name_).get<size_t>();

            tmp8 |= (raw << start_bit_) & bitmask8;
        }
        else if (data_type_ == "int")
        {
            long int raw;
            if (has_lsb_)
                raw = llround(source.at(name_).get<double>() / lsb_);
            else
                raw = source.at(name_).get<long int>();

            size_t masked = static_cast<size_t>(raw) & ((size_t(1) << bit_length_) - 1);
            tmp8 |= (masked << start_bit_) & bitmask8;
        }
        else if (data_type_ == "digits")
        {
            size_t digits_val = source.at(name_).get<size_t>();

            for (unsigned int cnt = 0; cnt < num_digits_; ++cnt)
            {
                size_t digit = digits_val % 10;
                digits_val /= 10;
                tmp8 |= (digit << (cnt * digit_bit_length_ + start_bit_)) & digits_bitmasks8[cnt];
            }
        }
        else if (data_type_ == "icao_characters" || data_type_ == "ascii_characters")
        {
            std::string str_val = source.at(name_).get<std::string>();

            for (unsigned int cnt = 0; cnt < num_characters_ && cnt < str_val.size(); ++cnt)
            {
                size_t code;
                if (data_type_ == "icao_characters")
                    code = getIcaoCode(str_val[str_val.size() - 1 - cnt]);
                else
                    code = static_cast<unsigned char>(str_val[str_val.size() - 1 - cnt]);

                tmp8 |= (code << (cnt * character_bit_length_ + start_bit_)) & chars_bitmasks8[cnt];
            }
        }

        // write back bytes
        for (int cnt = byte_length_ - 1; cnt >= 0; --cnt)
        {
            target[cnt] = static_cast<char>(tmp8 & 0xFF);
            tmp8 >>= 8;
        }
    }

    return 0;  // FixedBits does not advance byte offset
}

}  // namespace jASTERIX
