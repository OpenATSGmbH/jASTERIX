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

#include "optionalitemparser.h"

#include "logger.h"
#include "traced_assert.h"

using namespace std;
using namespace nlohmann;

namespace jASTERIX
{
OptionalItemParser::OptionalItemParser(const nlohmann::json& item_definition, const std::string& long_name_prefix)
    : ItemParserBase(item_definition, long_name_prefix)
{
    traced_assert(type_ == "optional_item");

    if (!item_definition.contains("optional_bitfield_name"))
        throw runtime_error("optional item '" + name_ + "' parsing without bitfield name");

    bitfield_name_ = item_definition.at("optional_bitfield_name");

    if (!item_definition.contains("optional_bitfield_index"))
        throw runtime_error("optional item '" + name_ + "' parsing without bitfield index");

    bitfield_index_ = item_definition.at("optional_bitfield_index");

    if (!item_definition.contains("data_fields"))
        throw runtime_error("parsing optional item '" + name_ + "' without sub-items");

    const json& data_fields = item_definition.at("data_fields");

    if (!data_fields.is_array())
        throw runtime_error("parsing optional item '" + name_ +
                            "' data fields container is not an array");

    std::string item_name;
    ItemParserBase* item{nullptr};

    for (const json& data_item_it : data_fields)
    {
        item_name = data_item_it.at("name");
        item = ItemParserBase::createItemParser(data_item_it, long_name_); // leave out own name
        traced_assert(item);
        data_fields_.push_back(std::unique_ptr<ItemParserBase>{item});
    }
}

size_t OptionalItemParser::parseItem(const char* data, size_t index, size_t size,
                                     size_t current_parsed_bytes, size_t total_size,
                                     nlohmann::json& target, bool debug)
{
    if (debug)
        loginf << "parsing optional item '" << name_ << "' index " << index << " size " << size
               << " current parsed bytes " << current_parsed_bytes << logendl;

    if (debug && !target.contains(bitfield_name_))
        throw runtime_error("parsing optional item '" + name_ + "' without defined bitfield '" +
                            bitfield_name_ + "'");

    const json& bitfield = target.at(bitfield_name_);

    if (debug && !bitfield.is_array())
        throw runtime_error("parsing optional item '" + name_ + "' with non-array bitfield '" +
                            bitfield_name_ + "'");

    if (bitfield_index_ >= bitfield.size())
    {
        if (debug)
            loginf << "parsing optional item '" << name_ << "' bitfield length " << bitfield.size()
                   << " index " << bitfield_index_ << " out of fspec size" << logendl;
        return 0;
    }

    if (debug && !bitfield.at(bitfield_index_).is_boolean())
        throw runtime_error("parsing optional item '" + name_ + "' with non-boolean bitfield '" +
                            bitfield_name_ + "' value");

    if (debug)
        loginf << "parsing optional item '" << name_ << "' bitfield length " << bitfield.size()
               << " index " << bitfield_index_ << logendl;

    bool item_exists = bitfield.at(bitfield_index_);

    if (debug)
        loginf << "parsing optional item '" << name_ << "' with " << data_fields_.size()
               << " data fields, exists " << item_exists << logendl;

    if (!item_exists)
        return 0;

    size_t parsed_bytes{0};

    if (debug)
        loginf << "parsing optional item '" + name_ + "' sub-items";

    for (auto& df_item : data_fields_)
    {
        parsed_bytes += df_item->parseItem(
                    data, index + parsed_bytes, size, current_parsed_bytes, total_size, target[name_], debug);
    }

    if (debug)
        loginf << "parsing optional item '" + name_ + "' done, " << parsed_bytes << " bytes parsed"
               << logendl;

    return parsed_bytes;
}

void OptionalItemParser::addInfo (const std::string& edition, CategoryItemInfo& info) const
{
    for (auto& item_it : data_fields_)
        item_it->addInfo(edition, info);
}

}  // namespace jASTERIX
