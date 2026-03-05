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

    if (!item_definition.contains("repetition_item"))
        throw runtime_error("repetitive item '" + name_ +
                            "' parsing without repetition item specification");

    const json& repetition_item = item_definition.at("repetition_item");

    if (!repetition_item.is_object())
        throw runtime_error("parsing repetitive item '" + name_ +
                            "' repetition item specification is not an object");

    if (repetition_item.at("name") != "REP")
        throw runtime_error("parsing repetitive item '" + name_ +
                            "' repetition item specification has to be named 'REP'");

    repetition_item_.reset(ItemParserBase::createItemParser(repetition_item, long_name_));
    traced_assert(repetition_item_);

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

    size_t parsed_bytes{0};

    if (index >= total_size)
        throw runtime_error("RepetetiveItemParser '" + name_ + "': REP byte at index " +
            to_string(index) + " exceeds total_size " + to_string(total_size));

    if (debug)
        loginf << "parsing repetitive item '" + name_ + "' repetition item" << logendl;

    // Read REP count directly from binary (always 1 byte unsigned per ASTERIX spec)
    unsigned int rep = static_cast<unsigned char>(data[index + parsed_bytes]);

    // Still call sub-parser to write REP to JSON for output
    parsed_bytes = repetition_item_->parseItem(
                data, index + parsed_bytes, size, parsed_bytes, total_size, target, debug);

    //traced_assert(!target.contains(name_));  // hot path — O(N) json lookup per field
    json& j_data = target[name_] = json::array();

    if (debug)
        loginf << "parsing repetitive item '" + name_ + "' items " << rep << " times" << logendl;

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

    return parsed_bytes;
}

size_t RepetetiveItemParser::encodeItem(const nlohmann::json& source, char* target,
                                        size_t max_size, bool debug)
{
    if (debug)
        loginf << "encoding repetitive item '" << name_ << "'" << logendl;

    size_t written_bytes{0};

    // encode REP byte
    written_bytes = repetition_item_->encodeItem(source, target, max_size, debug);

    // encode repeated items
    const json& j_array = source.at(name_);

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

}  // namespace jASTERIX
