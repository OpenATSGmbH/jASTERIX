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

#pragma once

#include "itemparserbase.h"

namespace jASTERIX
{
class FixedBitsItemParser : public ItemParserBase
{
  public:
    FixedBitsItemParser(const nlohmann::json& item_definition, const std::string& long_name_prefix,
                        unsigned int byte_length);
    virtual ~FixedBitsItemParser() {}

    virtual size_t parseItem(const char* data, size_t index, size_t size,
                             size_t current_parsed_bytes, size_t total_size,
                             nlohmann::json& target, bool debug) override;

  protected:
    unsigned int byte_length_{0};
    unsigned int start_bit_{0};
    unsigned int bit_length_{0};
    std::string data_type_{"uint"};
    //unsigned int negative_bit_pos_{0};
    bool has_lsb_{false};
    double lsb_{1.0};

    unsigned char bitmask1;
    unsigned int bitmask4;
    size_t bitmask8;

    unsigned char neg_bitmask1;
    unsigned int neg_bitmask4;
    size_t neg_bitmask8;

    // data type digits stuff
    unsigned int num_digits_{0};
    unsigned int digit_bit_length_{0};

    std::vector<unsigned char> digits_bitmasks1;
    std::vector<unsigned int> digits_bitmasks4;
    std::vector<size_t> digits_bitmasks8;

    // data type characters stuff

    unsigned int num_characters_{0};
    unsigned int character_bit_length_{0};

    std::vector<unsigned char> chars_bitmasks1;
    std::vector<unsigned int> chars_bitmasks4;
    std::vector<size_t> chars_bitmasks8;
};

}  // namespace jASTERIX


