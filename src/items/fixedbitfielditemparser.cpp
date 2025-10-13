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

#include "fixedbitfielditemparser.h"

#include "fixedbitsitemparser.h"
#include "logger.h"
#include "traced_assert.h"

using namespace std;
using namespace nlohmann;

namespace jASTERIX
{
FixedBitFieldItemParser::FixedBitFieldItemParser(const nlohmann::json& item_definition,
                                                 const std::string& long_name_prefix)
    : ItemParserBase(item_definition, long_name_prefix)
{
    traced_assert(type_ == "fixed_bitfield");

    optional_ = item_definition.contains("optional") && item_definition.at("optional") == true;

    if (optional_)
    {
        if (!item_definition.contains("optional_variable_name"))
            throw runtime_error("parsing fixed bitfield item '" + name_ +
                                "' optional but no variable given");

        optional_variable_name_ = item_definition.at("optional_variable_name");
        optional_variable_name_parts_ = split(optional_variable_name_, '.');

        if (!item_definition.contains("optional_variable_value"))
            throw runtime_error("parsing fixed bitfield item '" + name_ +
                                "' optional but no variable value given");

        optional_variable_value_ = item_definition.at("optional_variable_value");
    }

    if (!item_definition.contains("length"))
        throw runtime_error("parsing fixed bitfield item '" + name_ + "' without length");

    length_ = item_definition.at("length");

    if (length_ > 8)
        throw runtime_error("parsing fixed bitfield item '" + name_ + "' with too big length");

    if (!item_definition.contains("items"))
        throw runtime_error("parsing fixed bitfield item '" + name_ + "' without sub-items");

    const json& items = item_definition.at("items");

    if (!items.is_array())
        throw runtime_error("parsing fixed bitfield item '" + name_ +
                            "' sub-items specification is not an array");

    std::string item_name;
    FixedBitsItemParser* item{nullptr};

    for (const json& data_item_it : items)
    {
        item_name = data_item_it.at("name");
        item = new FixedBitsItemParser(data_item_it, long_name_prefix_, length_); // leave out own name
        traced_assert(item);
        items_.push_back(std::unique_ptr<ItemParserBase>{item});
    }
}

size_t FixedBitFieldItemParser::parseItem(const char* data, size_t index, size_t size,
                                          size_t current_parsed_bytes, size_t total_size,
                                          nlohmann::json& target, bool debug)
{
    if (debug)
        loginf << "parsing fixed bitfield item '" << name_ << "' index " << index << " size "
               << size << " current parsed bytes " << current_parsed_bytes << logendl;

    if (optional_ &&
        !variableHasValue(target, optional_variable_name_parts_, optional_variable_value_))
    {
        if (debug)  //  in '" << parent.dump(4) << "'"
            loginf << "parsing fixed bitfield item '" << name_ << "' skipped since variable '"
                   << optional_variable_name_ << "' not set in '" << target.dump(4) << "'"
                   << logendl;

        return 0;  // no parse
    }

    for (auto& sub_item_it : items_)
    {
        if (debug)
            loginf << "parsing fixed bitfield item '" << name_ << "' item '" << sub_item_it->name()
                   << "'" << logendl;

        sub_item_it->parseItem(data, index, size, current_parsed_bytes, total_size, target, debug);
    }

    return length_;
}


void FixedBitFieldItemParser::addInfo (const std::string& edition, CategoryItemInfo& info) const
{
    for (auto& item_it : items_)
        item_it->addInfo(edition, info);
}

}  // namespace jASTERIX
