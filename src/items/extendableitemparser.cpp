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

#include "extendableitemparser.h"

#include "logger.h"
#include "traced_assert.h"

using namespace std;
using namespace nlohmann;

namespace jASTERIX
{
ExtendableItemParser::ExtendableItemParser(const nlohmann::json& item_definition, const std::string& long_name_prefix)
    : ItemParserBase(item_definition, long_name_prefix)
{
    traced_assert(type_ == "extendable");

    if (!item_definition.contains("items"))
        throw runtime_error("parsing extendable item '" + name_ + "' without items");

    const json& items = item_definition.at("items");

    if (!items.is_array())
        throw runtime_error("parsing extendable item '" + name_ +
                            "' items specification is not an array");

    std::string item_name;
    ItemParserBase* item{nullptr};

    for (const json& data_item_it : items)
    {
        item_name = data_item_it.at("name");
        item = ItemParserBase::createItemParser(data_item_it, long_name_); // leave out own name
        traced_assert(item);
        items_.push_back(std::unique_ptr<ItemParserBase>{item});
    }
}

size_t ExtendableItemParser::parseItem(const char* data, size_t index, size_t size,
                                       size_t current_parsed_bytes, size_t total_size,
                                       nlohmann::json& target, bool debug)
{
    if (debug)
        loginf << "parsing extendable item '" << name_ << "' with " << items_.size()
               << " items index " << index << " size " << size << " current parsed bytes "
               << current_parsed_bytes << logendl;

    size_t parsed_bytes{0};

    if (debug)
        loginf << "parsing extendable item '" + name_ + "' items" << logendl;

    unsigned int extend = 1;
    unsigned int cnt = 0;

    traced_assert(!target.contains(name_));
    json& j_data = target[name_] = json::array();

    while (extend)
    {
        for (auto& data_item_it : items_)
        {
            if (debug)
                loginf << "parsing extendable item '" << name_ << "' data item '"
                       << data_item_it->name() << "' index " << index + parsed_bytes << " cnt "
                       << cnt << logendl;

            json& current = j_data[cnt];
            parsed_bytes += data_item_it->parseItem(
                        data, index + parsed_bytes, size, parsed_bytes, total_size, current, debug);

            if (debug && !current.contains("extend"))
                throw runtime_error("parsing extendable item '" + name_ +
                                    "' without extend information");

            extend = current.at("extend");

            ++cnt;
        }
    }

    return parsed_bytes;
}

void ExtendableItemParser::addInfo (const std::string& edition, CategoryItemInfo& info) const
{
    for (auto& item_it : items_)
        item_it->addInfo(edition, info);
}

}  // namespace jASTERIX
