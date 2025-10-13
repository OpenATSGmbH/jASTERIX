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

#include "ref.h"

#include "logger.h"
#include "string_conv.h"
#include "traced_assert.h"

using namespace std;
using namespace nlohmann;

namespace jASTERIX
{
ReservedExpansionField::ReservedExpansionField(const nlohmann::json& item_definition)
    : ItemParserBase(item_definition)
{
    traced_assert(type_ == "ReservedExpansionField");

    // fspec

    if (!item_definition.contains("field_specification"))
        throw runtime_error("ReservedExpansionField item '" + name_ +
                            "' parsing without field specification");

    const json& field_specification = item_definition.at("field_specification");

    if (!field_specification.is_object())
        throw runtime_error("ReservedExpansionField item '" + name_ +
                            "' field specification is not an object");

    field_specification_.reset(ItemParserBase::createItemParser(field_specification, "REF"));
    traced_assert(field_specification_);

    // uap

    if (!item_definition.contains("items_indicator"))
        throw runtime_error("ReservedExpansionField item '" + name_ +
                            "' without items indicator definition");

    const json& items_indicator = item_definition.at("items_indicator");

    if (!items_indicator.is_array())
        throw runtime_error("ReservedExpansionField item '" + name_ +
                            "' items indicator definition is not an array");

    for (const auto& item_def : items_indicator)
        items_names_.push_back(item_def);

    // items

    if (!item_definition.contains("items"))
        throw runtime_error("ReservedExpansionField item '" + name_ + "' without items");

    const json& items = item_definition.at("items");

    if (!items.is_array())
        throw runtime_error("ReservedExpansionField item '" + name_ +
                            "' field specification is not an array");

    std::string item_number;
    ItemParserBase* item{nullptr};

    for (const json& data_item_it : items)
    {
        if (!data_item_it.contains("number"))
            throw runtime_error("ReservedExpansionField item '" + data_item_it.dump(4) +
                                "' without number");

        item_number = data_item_it.at("number");
        item = ItemParserBase::createItemParser(data_item_it, "REF");
        traced_assert(item);

        if (items_.count(item_number) != 0)
            throw runtime_error("ReservedExpansionField item '" + name_ + "' item number '" +
                                item_number + "' used multiple times");

        items_[item_number] = std::unique_ptr<ItemParserBase>{item};
    }
}

ReservedExpansionField::~ReservedExpansionField() {}

size_t ReservedExpansionField::parseItem(const char* data, size_t index, size_t size,
                                         size_t current_parsed_bytes, size_t total_size,
                                         nlohmann::json& target, bool debug)
{
    if (debug)
        loginf << "parsing ReservedExpansionField item '" << name_ << "' with index " << index
               << " size " << size << " current parsed bytes " << current_parsed_bytes << logendl;

    size_t parsed_bytes{0};

    if (debug)
        loginf << "parsing ReservedExpansionField item '" + name_ + "' field specification"
               << logendl;

    parsed_bytes = field_specification_->parseItem(
                data, index + parsed_bytes, size, parsed_bytes, total_size, target, debug);

    if (!target.contains("REF_FSPEC"))
        throw runtime_error("ReservedExpansionField item '" + name_ + "' FSPEC not found");

    std::vector<bool> fspec_bits = target.at("REF_FSPEC");

    //    if (!has_conditional_uap_ && fspec_bits.size() > uap_names_.size())
    //        throw runtime_error ("ReservedExpansionField item '"+name_+"' has more FSPEC bits than
    //        defined uap items");

    if (debug)
        loginf << "parsing ReservedExpansionField item '" + name_ + "' data items" << logendl;

    size_t uap_cnt{0};
    size_t num_fspec_bits = fspec_bits.size();

    //    bool special_purpose_field_present {false};
    //    bool reserved_expansion_field_present {false};

    for (const auto& item_name : items_names_)  // parse static uap items
    {
        if (uap_cnt >= num_fspec_bits)
            break;

        if (fspec_bits.at(uap_cnt))
        {
            uap_cnt++;

            if (item_name == "FX")  // extension into next byte
                continue;

            if (item_name == "-")  // bit not used
                continue;

            if (debug)
                loginf << "parsing ReservedExpansionField item '" << name_ << "' data item '"
                       << item_name << "' index " << index + parsed_bytes << logendl;

            if (items_.count(item_name) != 1)
                throw runtime_error("ReservedExpansionField item '" + name_ +
                                    "' references undefined item '" + item_name + "'");

            parsed_bytes += items_.at(item_name)->parseItem(
                        data, index + parsed_bytes, size, parsed_bytes, total_size, target, debug);
        }
        else
            uap_cnt++;
    }

    return parsed_bytes;
}

void ReservedExpansionField::addInfo (const std::string& edition, CategoryItemInfo& info) const
{
    for (auto& item_it : items_)
        item_it.second->addInfo(edition, info);
}

// bool ReservedExpansionField::compareKey (const nlohmann::json& container, const std::string&
// value)
//{
//    //loginf << "mapping key '" << key_definition << "' src value '" << src_value << "'";

//    const nlohmann::json* val_ptr = &container;

//    for (const std::string& sub_key : conditional_uaps_sub_keys_)
//    {
//        //loginf << "UGA '" << sub_key << "' json '" << val_ptr->dump(4) << "'"<< logendl;
//        val_ptr = &(*val_ptr)[sub_key];
//    }

//    return (toString(*val_ptr) == value);

//    //loginf << "mapping key '" << key_definition << "' dest '" << dest.dump(4) << "'";
//}

}  // namespace jASTERIX
