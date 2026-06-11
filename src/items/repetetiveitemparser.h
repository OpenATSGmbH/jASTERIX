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
class RepetetiveItemParser : public ItemParserBase
{
  public:
    RepetetiveItemParser(const nlohmann::json& item_definition, const std::string& long_name_prefix);
    virtual ~RepetetiveItemParser() {}

    virtual size_t parseItem(const char* data, size_t index, size_t size,
                             size_t current_parsed_bytes, size_t total_size,
                             nlohmann::json& target, bool debug) override;

    virtual size_t encodeItem(const nlohmann::json& source, char* target,
                              size_t max_size, bool debug) override;

    virtual void addInfo (const std::string& edition, CategoryItemInfo& info) const override;

    virtual void setupColumnWriters(const LeafSetupCallback& callback) override;

  protected:
    std::vector<std::unique_ptr<ItemParserBase>> items_;

    // Column for the REP count ('<prefix>.REP') in columnar mode
    nlohmann::json* rep_column_target_ = nullptr;
};

}  // namespace jASTERIX

