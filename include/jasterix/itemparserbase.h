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

#include <jasterix/iteminfo.h>

#include "json.hpp"

#include <cstddef>
#include <sstream>
#include <cassert>
#include <string>
#include <vector>

namespace jASTERIX
{
class ItemParserBase
{
public:
    ItemParserBase(const nlohmann::json& item_definition, const std::string& long_name_prefix="");
    virtual ~ItemParserBase() {}

    static ItemParserBase* createItemParser(const nlohmann::json& item_definition,
                                            const std::string& long_name_prefix);

    // Parse binary ASTERIX data into JSON. Returns number of bytes consumed.
    // @param data                Pointer to the full binary data buffer
    // @param index               Absolute byte offset where this item starts in 'data'
    // @param size                Remaining bytes in the current data block content
    // @param current_parsed_bytes  Bytes parsed so far within the enclosing record/REF/SPF
    // @param total_size          Total buffer size — hard upper bound for any data[offset] access
    // @param target              JSON object to write parsed values into
    // @param debug               Enable debug logging
    virtual size_t parseItem(const char* data, size_t index, size_t size,
                             size_t current_parsed_bytes, size_t total_size,
                             nlohmann::json& target, bool debug) = 0;

    // Encode JSON back to binary ASTERIX. Returns number of bytes written.
    // @param source    JSON object containing the values to encode
    // @param target    Pre-allocated output buffer to write binary data into
    // @param max_size  Available space in the target buffer
    // @param debug     Enable debug logging
    virtual size_t encodeItem(const nlohmann::json& source, char* target,
                              size_t max_size, bool debug) = 0;
    std::string name() const;
    std::string longNamePrefix() const;
    std::string longName() const;
    std::string type() const;

    virtual void addInfo (const std::string& edition, CategoryItemInfo& info) const;

protected:
    const nlohmann::json& item_definition_;
    std::string name_;
    std::string long_name_prefix_;
    std::string long_name_;
    std::string type_;
};

bool variableHasValue(const nlohmann::json& data,
                      const std::vector<std::string>& variable_name_parts,
                      const nlohmann::json& variable_value);

inline unsigned char reverseBits(unsigned char b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;

    return b;
}

inline std::vector<std::string> split(const std::string& s, char delim)
{
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

}  // namespace jASTERIX


