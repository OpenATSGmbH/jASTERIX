/*
 * This file is part of ATSDB.
 *
 * ATSDB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ATSDB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with ATSDB.  If not, see <http://www.gnu.org/licenses/>.
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

    unsigned char tmp1{0};

    if (byte_length_ == 1)
    {
        tmp1 = *reinterpret_cast<const unsigned char*>(&data[index]);

        if (data_type_ == "uint")
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
        }
        else if (data_type_ == "int")
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
        }
        else if (data_type_ == "digits")
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
        }
        else if (data_type_ == "icao_characters" || data_type_ == "ascii_characters")
        {
            string characters_tmp;
            char char_tmp1;

            for (int cnt = num_characters_ - 1; cnt >= 0; --cnt)
            {
                char_tmp1 = tmp1 & chars_bitmasks1[cnt];
                char_tmp1 >>= cnt * character_bit_length_;

                if (data_type_ == "icao_characters")
                    characters_tmp += getIcaoChar(char_tmp1);
                else
                    characters_tmp += char_tmp1;

                if (debug)
                    loginf << "parsing fixed bits item '" << name_ << "' type " << data_type_
                           << " cnt " << cnt << " characters tmp '" << characters_tmp << "' value "
                           << (size_t)char_tmp1 << " bitmask " << chars_bitmasks1[cnt] << logendl;
            }
            target.emplace(name_, characters_tmp);
        }
        else
            throw runtime_error("fixed bits item '" + name_ + "' parsing with unknown data type '" +
                                data_type_ + "'");
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

        if (data_type_ == "uint")
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
        }
        else if (data_type_ == "int")
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
        }
        else if (data_type_ == "digits")
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
        }
        else if (data_type_ == "icao_characters" || data_type_ == "ascii_characters")
        {
            string characters_tmp;
            size_t char_tmp4;

            for (int cnt = num_characters_ - 1; cnt >= 0; --cnt)
            {
                char_tmp4 = tmp4 & chars_bitmasks4[cnt];
                char_tmp4 >>= cnt * character_bit_length_;

                if (data_type_ == "icao_characters")
                    characters_tmp += getIcaoChar(char_tmp4);
                else
                    characters_tmp += static_cast<unsigned char>(char_tmp4);

                if (debug)
                    loginf << "parsing fixed bits item '" << name_ << "' type characters cnt "
                           << cnt << " characters tmp '" << characters_tmp << "' value "
                           << (size_t)char_tmp4 << " bitmask " << chars_bitmasks4[cnt] << logendl;
            }
            target.emplace(name_, characters_tmp);
        }
        else
            throw runtime_error("fixed bits item '" + name_ + "' parsing with unknown data type '" +
                                data_type_ + "'");
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
        if (data_type_ == "uint")
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
        }
        else if (data_type_ == "int")
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
        }
        else if (data_type_ == "digits")
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
        }
        else if (data_type_ == "icao_characters" || data_type_ == "ascii_characters")
        {
            string characters_tmp;
            size_t char_tmp8;

            for (int cnt = num_characters_ - 1; cnt >= 0; --cnt)
            {
                char_tmp8 = tmp8 & chars_bitmasks8[cnt];
                char_tmp8 >>= cnt * character_bit_length_;

                if (data_type_ == "icao_characters")
                    characters_tmp += getIcaoChar(char_tmp8);
                else
                    characters_tmp += static_cast<unsigned char>(char_tmp8);

                if (debug)
                    loginf << "parsing fixed bits item '" << name_ << "' type characters cnt "
                           << cnt << " characters tmp '" << characters_tmp << "' value "
                           << (size_t)char_tmp8 << " bitmask " << chars_bitmasks8[cnt] << logendl;
            }
            target.emplace(name_, characters_tmp);
        }
        else
            throw runtime_error("fixed bits item '" + name_ + "' parsing with unknown data type '" +
                                data_type_ + "'");
    }

    return 0;
}

}  // namespace jASTERIX
