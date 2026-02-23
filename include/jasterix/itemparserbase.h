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

    // always return number of parsed bytes
    // size can be for sub-item parsing and relative to index (already parsed),
    // total size always largest possible, e.g. file size
    virtual size_t parseItem(const char* data, size_t index, size_t size,
                             size_t current_parsed_bytes, size_t total_size,
                             nlohmann::json& target, bool debug) = 0;
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


