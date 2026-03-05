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
class FixedBytesItemParser : public ItemParserBase
{
  public:
    enum class DataType { String, Uint, Int, Bin };

    FixedBytesItemParser(const nlohmann::json& item_definition, const std::string& long_name_prefix);
    virtual ~FixedBytesItemParser() {}

    virtual size_t parseItem(const char* data, size_t index, size_t size,
                             size_t current_parsed_bytes, size_t total_size,
                             nlohmann::json& target, bool debug) override;

    virtual size_t encodeItem(const nlohmann::json& source, char* target,
                              size_t max_size, bool debug) override;

  protected:
    size_t length_{0};
    std::string data_type_;
    DataType data_type_enum_;
    bool reverse_bits_{false};
    bool reverse_bytes_{false};
    unsigned int negative_bit_pos_{0};
    bool has_lsb_{false};
    double lsb_{1.0};
};

}  // namespace jASTERIX


