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

#include "skipbytesitemparser.h"

#include "logger.h"
#include "traced_assert.h"

using namespace std;
using namespace nlohmann;

namespace jASTERIX
{
SkipBytesItemParser::SkipBytesItemParser(const nlohmann::json& item_definition, const std::string& long_name_prefix)
    : ItemParserBase(item_definition, long_name_prefix)
{
    traced_assert(type_ == "skip_bytes");

    if (!item_definition.contains("length"))
        throw runtime_error("fixed bytes item '" + name_ + "' parsing without length");

    length_ = item_definition.at("length");
}

size_t SkipBytesItemParser::parseItem(const char* data, size_t index, size_t size,
                                      size_t current_parsed_bytes, size_t total_size, nlohmann::json& target,
                                      bool debug)
{
    if (debug)
        loginf << "parsing skipped bytes item '" + name_ + "' index " << index << " length "
               << length_ << " index " << index << " size " << size << " current parsed bytes "
               << current_parsed_bytes << logendl;

    return length_;
}

}  // namespace jASTERIX
