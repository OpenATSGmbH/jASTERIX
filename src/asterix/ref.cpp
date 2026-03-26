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
#include "extendablebitsitemparser.h"

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

    auto* fspec_parser = static_cast<ExtendableBitsItemParser*>(field_specification_.get());
    std::vector<bool> fspec_bits;
    parsed_bytes = fspec_parser->parseItemBits(
                data, index + parsed_bytes, size, parsed_bytes, total_size, fspec_bits, debug,
                &items_names_);

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

            if (index + parsed_bytes >= total_size)
                throw runtime_error("ReservedExpansionField '" + name_ + "': at index " +
                    to_string(index + parsed_bytes) + " exceeds total_size " + to_string(total_size));

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

size_t ReservedExpansionField::encodeItem(const nlohmann::json& source, char* target,
                                          size_t max_size, bool debug)
{
    if (debug)
        loginf << "encoding ReservedExpansionField item '" << name_ << "'" << logendl;

    // Reconstruct REF_FSPEC from which items are present in source
    std::vector<bool> fspec_bits;
    fspec_bits.resize(items_names_.size(), false);

    size_t uap_cnt = 0;
    for (const auto& item_name : items_names_)
    {
        if (item_name == "FX" || item_name == "-")
        {
            uap_cnt++;
            continue;
        }
        fspec_bits[uap_cnt] = (items_.count(item_name) == 1 && source.contains(item_name));
        uap_cnt++;
    }

    // Pad to full bytes and set FX bits only where items_names has an FX entry
    size_t num_bytes = (fspec_bits.size() + 7) / 8;
    fspec_bits.resize(num_bytes * 8, false);

    // check if the indicator uses FX extension at all
    bool has_fx = false;
    for (size_t i = 7; i < items_names_.size(); i += 8)
        if (items_names_.at(i).substr(0, 2) == "FX")
        { has_fx = true; break; }

    if (has_fx)
    {
        size_t last_needed_byte = 0;
        for (size_t byte_idx = 0; byte_idx < num_bytes; ++byte_idx)
            for (size_t bit = 0; bit < 7; ++bit)
                if (fspec_bits[byte_idx * 8 + bit])
                    last_needed_byte = byte_idx;

        for (size_t byte_idx = 0; byte_idx < last_needed_byte; ++byte_idx)
            fspec_bits[byte_idx * 8 + 7] = true;
        fspec_bits[last_needed_byte * 8 + 7] = false;
        fspec_bits.resize((last_needed_byte + 1) * 8);
    }

    auto* fspec_parser = static_cast<ExtendableBitsItemParser*>(field_specification_.get());
    size_t written_bytes = fspec_parser->encodeBits(fspec_bits, target, max_size, debug);

    uap_cnt = 0;
    size_t num_fspec_bits = fspec_bits.size();

    for (const auto& item_name : items_names_)
    {
        if (uap_cnt >= num_fspec_bits)
            break;

        if (fspec_bits.at(uap_cnt))
        {
            uap_cnt++;

            if (item_name == "FX" || item_name == "-")
                continue;

            if (items_.count(item_name) != 1)
                throw runtime_error("ReservedExpansionField item '" + name_ +
                                    "' references undefined item '" + item_name + "'");

            written_bytes += items_.at(item_name)->encodeItem(
                source, target + written_bytes, max_size - written_bytes, debug);
        }
        else
            uap_cnt++;
    }

    return written_bytes;
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
