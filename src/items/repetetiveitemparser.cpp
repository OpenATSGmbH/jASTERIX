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

#include "repetetiveitemparser.h"

#include "logger.h"
#include "traced_assert.h"

using namespace std;
using namespace nlohmann;

namespace jASTERIX
{
RepetetiveItemParser::RepetetiveItemParser(const nlohmann::json& item_definition, const std::string& long_name_prefix)
    : ItemParserBase(item_definition, long_name_prefix)
{
    traced_assert(type_ == "repetitive");

    // REP byte is always 1 unsigned byte per ASTERIX spec — no sub-parser needed

    if (!item_definition.contains("items"))
        throw runtime_error("parsing repetitive item '" + name_ + "' without items");

    const json& items = item_definition.at("items");

    if (!items.is_array())
        throw runtime_error("parsing repetitive item '" + name_ +
                            "' items specification is not an array");

    std::string item_name;
    ItemParserBase* item{nullptr};

    for (const json& data_item_it : items)
    {
        item_name = data_item_it.at("name");
        item = ItemParserBase::createItemParser(data_item_it, long_name_);
        traced_assert(item);
        items_.push_back(std::unique_ptr<ItemParserBase>{item});
    }
}

size_t RepetetiveItemParser::parseItem(const char* data, size_t index, size_t size,
                                       size_t current_parsed_bytes, size_t total_size,
                                       nlohmann::json& target, bool debug)
{
    if (debug)
        loginf << "parsing repetitive item '" << name_ << "' with " << items_.size()
               << " items index " << index << " size " << size << " current parsed bytes "
               << current_parsed_bytes << logendl;

    if (index >= total_size)
        throw runtime_error("RepetetiveItemParser '" + name_ + "': REP byte at index " +
            to_string(index) + " exceeds total_size " + to_string(total_size));

    // Read REP count directly from binary (always 1 byte unsigned per ASTERIX spec)
    unsigned int rep = static_cast<unsigned char>(data[index]);
    size_t parsed_bytes = 1;  // consume the REP byte

    if (debug)
        loginf << "parsing repetitive item '" + name_ + "' items " << rep << " times" << logendl;

    if (column_target_)
    {
        if (rep_column_target_)
            (*rep_column_target_)[*record_index_] = rep;

        json arr = json::array();
        for (unsigned int cnt = 0; cnt < rep; ++cnt)
        {
            json elem = json::object();
            for (auto& data_item_it : items_)
            {
                if (debug)
                    loginf << "parsing repetitive item '" << name_ << "' data item '"
                           << data_item_it->name() << "' index " << index + parsed_bytes << " cnt "
                           << cnt << logendl;

                parsed_bytes += data_item_it->parseItem(
                            data, index + parsed_bytes, size, parsed_bytes, total_size, elem, debug);
            }
            arr.push_back(std::move(elem));
        }
        (*column_target_)[*record_index_] = std::move(arr);
    }
    else
    {
        target["REP"] = rep;

        json& j_data = target[name_] = json::array();

        for (unsigned int cnt = 0; cnt < rep; ++cnt)
        {
            for (auto& data_item_it : items_)
            {
                if (debug)
                    loginf << "parsing repetitive item '" << name_ << "' data item '"
                           << data_item_it->name() << "' index " << index + parsed_bytes << " cnt "
                           << cnt << logendl;

                parsed_bytes += data_item_it->parseItem(
                            data, index + parsed_bytes, size, parsed_bytes, total_size, j_data[cnt], debug);
            }
        }
    }

    return parsed_bytes;
}

size_t RepetetiveItemParser::encodeItem(const nlohmann::json& source, char* target,
                                        size_t max_size, bool debug)
{
    if (debug)
        loginf << "encoding repetitive item '" << name_ << "'" << logendl;

    // encode repeated items
    const json& j_array = source.at(name_);

    // Write REP byte directly
    target[0] = static_cast<char>(j_array.size());
    size_t written_bytes = 1;

    for (size_t cnt = 0; cnt < j_array.size(); ++cnt)
    {
        const json& element = j_array[cnt];

        for (auto& data_item_it : items_)
        {
            written_bytes += data_item_it->encodeItem(element, target + written_bytes,
                                                      max_size - written_bytes, debug);
        }
    }

    return written_bytes;
}

void RepetetiveItemParser::addInfo (const std::string& edition, CategoryItemInfo& info) const
{
    for (auto& item_it : items_)
        item_it->addInfo(edition, info);
}

void RepetetiveItemParser::setupColumnWriters(const LeafSetupCallback& callback)
{
    callback(this, long_name_);

    // extra column for the REP count, e.g. 'REF.CSN.REP' next to 'REF.CSN.CSN'
    std::string rep_name = long_name_prefix_.size() ? long_name_prefix_ + ".REP" : "REP";
    rep_column_target_ = callback(nullptr, rep_name);

    // Do NOT recurse into children — they write structured into each repetition element
}

}  // namespace jASTERIX
