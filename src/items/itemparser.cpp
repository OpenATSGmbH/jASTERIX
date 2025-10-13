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

#include "itemparser.h"

#include "logger.h"
#include "traced_assert.h"

using namespace std;
using namespace nlohmann;

namespace jASTERIX
{
ItemParser::ItemParser(const nlohmann::json& item_definition, const std::string& long_name_prefix)
    : ItemParserBase(item_definition, long_name_prefix)
{
    traced_assert(type_ == "item");

    if (!item_definition.contains("number"))
        throw runtime_error("parsing item '" + name_ + "' without number");

    number_ = item_definition.at("number");

    if (long_name_prefix_.size())
        long_name_ = long_name_prefix_ + "." + number_;
    else
        long_name_ = number_;

    const json& data_fields = item_definition.at("data_fields");

    if (!data_fields.is_array())
        throw runtime_error("parsing item '" + name_ + "' data fields container is not an array");

    std::string item_name;
    std::string item_number;
    ItemParserBase* item{nullptr};

    for (const json& data_item_it : data_fields)
    {
        item_name = data_item_it.at("name");

        if (data_item_it.contains("number"))
            item_number = data_item_it.at("number");
        else
            item_number = "";

        item = ItemParserBase::createItemParser(data_item_it, long_name_);
        traced_assert(item);
        data_fields_.push_back(std::unique_ptr<ItemParserBase>{item});
    }
}

size_t ItemParser::parseItem(const char* data, size_t index, size_t size,
                             size_t current_parsed_bytes, size_t total_size, nlohmann::json& target, bool debug)
{
    if (debug)
        loginf << "parsing item '" << name_ << "'" << logendl;

    size_t parsed_bytes{0};

    for (auto& df_item : data_fields_)
    {
        parsed_bytes += df_item->parseItem(
                    data, index + parsed_bytes, size, current_parsed_bytes, total_size, target[number_], debug);
    }

    if (debug)
        loginf << "parsing item '" + name_ + "' done, " << parsed_bytes << " bytes parsed"
               << logendl;

    return parsed_bytes;
}

std::string ItemParser::number() const { return number_; }

void ItemParser::addInfo (const std::string& edition, CategoryItemInfo& info) const
{
    for (auto& field_it : data_fields_)
        field_it->addInfo(edition, info);
}


}  // namespace jASTERIX
