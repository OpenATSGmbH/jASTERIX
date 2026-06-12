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

#include <jasterix/itemparserbase.h>

#include <memory>
#include <vector>

namespace jASTERIX
{
// decodes a field specification/availablity field (ending with extend bit), and list of items
class SpecialPurposeField : public ItemParserBase
{
  public:
    SpecialPurposeField(const nlohmann::json& item_definition);
    virtual ~SpecialPurposeField() override;

    virtual size_t parseItem(const char* data, size_t index, size_t size,
                             size_t current_parsed_bytes, size_t total_size, nlohmann::json& target,
                             bool debug) override;

    virtual size_t encodeItem(const nlohmann::json& source, char* target,
                              size_t max_size, bool debug) override;

    virtual void addInfo (const std::string& edition, CategoryItemInfo& info) const override;

    virtual void setupColumnWriters(const LeafSetupCallback& callback) override;

  protected:
    std::unique_ptr<ItemParserBase> complex_field_specification_;
    std::vector<std::string> complex_items_names_;
    std::map<std::string, std::unique_ptr<ItemParserBase>> complex_items_;

    std::vector<std::unique_ptr<ItemParserBase>> simple_items_;

    size_t parseSimpleItem(const char* data, size_t index, size_t size,
                           size_t current_parsed_bytes, size_t total_size, nlohmann::json& target, bool debug);

    size_t parseComplexItem(const char* data, size_t index, size_t size,
                            size_t current_parsed_bytes, size_t total_size, nlohmann::json& target, bool debug);
};

}  // namespace jASTERIX


