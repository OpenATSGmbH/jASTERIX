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
// parses all bits per byte into array<bool>, the last of each byte signifying the extension into
// next byte
class ExtendableBitsItemParser : public ItemParserBase
{
  public:
    ExtendableBitsItemParser(const nlohmann::json& item_definition, const std::string& long_name_prefix);
    virtual ~ExtendableBitsItemParser() {}

    virtual size_t parseItem(const char* data, size_t index, size_t size,
                             size_t current_parsed_bytes, size_t total_size,
                             nlohmann::json& target, bool debug) override;

    // Parse FX-extended bitmask into a local vector without writing to JSON.
    // If items_names is provided, extension only occurs when the last bit of each
    // byte corresponds to an entry starting with "FX"; otherwise the byte is final.
    size_t parseItemBits(const char* data, size_t index, size_t size,
                         size_t current_parsed_bytes, size_t total_size,
                         std::vector<bool>& out_bits, bool debug,
                         const std::vector<std::string>* items_names = nullptr);

    // Encode from a pre-built bitfield (when not stored in JSON)
    size_t encodeBits(const std::vector<bool>& bits, char* target,
                      size_t max_size, bool debug);

    virtual size_t encodeItem(const nlohmann::json& source, char* target,
                              size_t max_size, bool debug) override;

  protected:
    std::string data_type_;
    bool reverse_bits_{false};
    bool reverse_order_{false};
};

}  // namespace jASTERIX


