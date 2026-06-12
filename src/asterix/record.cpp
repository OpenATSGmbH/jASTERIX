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

#include "record.h"
#include "extendablebitsitemparser.h"

#include "logger.h"
#include "string_conv.h"
#include "traced_assert.h"

using namespace std;
using namespace nlohmann;

namespace jASTERIX
{
Record::Record(const nlohmann::json& item_definition) : ItemParserBase(item_definition)
{
    traced_assert(type_ == "record");

    // fspec

    if (!item_definition.contains("field_specification"))
        throw runtime_error("record item '" + name_ + "' parsing without field specification");

    const json& field_specification = item_definition.at("field_specification");

    if (!field_specification.is_object())
        throw runtime_error("record item '" + name_ + "' field specification is not an object");

    field_specification_.reset(ItemParserBase::createItemParser(field_specification, ""));
    traced_assert(field_specification_);

    // uap

    if (!item_definition.contains("uap"))
        throw runtime_error("record item '" + name_ + "' without uap");

    const json& uap = item_definition.at("uap");

    if (!uap.is_array())
        throw runtime_error("record item '" + name_ + "' uap is not an array");

    for (const auto& uap_item : uap)
        uap_names_.push_back(uap_item);

    max_fspec_bits_ = uap.size();

    // conditional uaps

    if (item_definition.contains("conditional_uaps"))
    {
        has_conditional_uap_ = true;
        const json& conditional_uaps = item_definition.at("conditional_uaps");

        if (!conditional_uaps.is_object())
            throw runtime_error("record item '" + name_ + "' conditional uaps is not an object");

        if (!conditional_uaps.contains("key"))
            throw runtime_error("record item '" + name_ + "' conditional uap without key");

        conditional_uaps_key_ = conditional_uaps.at("key");

        conditional_uaps_sub_keys_ = split(conditional_uaps_key_, '.');

        if (!conditional_uaps.contains("values"))
            throw runtime_error("record item '" + name_ + "' conditional uap without values");

        const json& conditional_uaps_values = conditional_uaps.at("values");

        if (!conditional_uaps_values.is_object())
            throw runtime_error("record item '" + name_ +
                                "' conditional uap values is not an object");

        unsigned int max_cond_fspec_bits{0};

        for (auto uap_conditional_value = conditional_uaps_values.begin();
             uap_conditional_value != conditional_uaps_values.end(); ++uap_conditional_value)
        {
            std::string value_key = uap_conditional_value.key();

            if (conditional_uap_names_.count(value_key) != 0)
                throw std::runtime_error("record item '" + name_ +
                                         "' conditional uap values has multiple uaps with"
                                         " same key");

            if (!uap_conditional_value.value().is_array())
                throw std::runtime_error("record item '" + name_ +
                                         "' conditional uap values has non-array values");

            std::vector<std::string> value_uap = uap_conditional_value.value();

            conditional_uap_names_[value_key] = value_uap;

            max_cond_fspec_bits = std::max(max_cond_fspec_bits, (unsigned int) value_uap.size());
        }

        max_fspec_bits_ += max_cond_fspec_bits;
    }

    // items

    if (!item_definition.contains("items"))
        throw runtime_error("record item '" + name_ + "' without items");

    const json& items = item_definition.at("items");

    if (!items.is_array())
        throw runtime_error("record item '" + name_ + "' field specification is not an array");

    std::string item_number;
    ItemParserBase* item{nullptr};

    for (const json& data_item_it : items)
    {
        if (!data_item_it.contains("number"))
            throw runtime_error("record item '" + data_item_it.dump(4) + "' without number");

        item_number = data_item_it.at("number");
        item = ItemParserBase::createItemParser(data_item_it, "");
        traced_assert(item);

        if (items_.count(item_number) != 0)
            throw runtime_error("record item '" + name_ + "' item number '" + item_number +
                                "' used multiple times");

        items_[item_number] = std::unique_ptr<ItemParserBase>{item};
    }

    // Build flat lookup vectors for O(1) UAP item access during parsing
    for (const auto& uap_name : uap_names_)
    {
        if (uap_name == "FX" || uap_name == "-" || uap_name == "SP" || uap_name == "RE")
            uap_items_.push_back(nullptr);
        else
        {
            auto it = items_.find(uap_name);
            uap_items_.push_back(it != items_.end() ? it->second.get() : nullptr);
        }
    }

    for (auto& [key, cond_names] : conditional_uap_names_)
    {
        auto& cond_items = conditional_uap_items_[key];
        for (const auto& uap_name : cond_names)
        {
            if (uap_name == "FX" || uap_name == "-" || uap_name == "SP" || uap_name == "RE")
                cond_items.push_back(nullptr);
            else
            {
                auto it = items_.find(uap_name);
                cond_items.push_back(it != items_.end() ? it->second.get() : nullptr);
            }
        }
    }
}

size_t Record::parseItem(const char* data, size_t index, size_t size, size_t current_parsed_bytes, size_t total_size,
                         nlohmann::json& target, bool debug)
{
    if (debug)
        loginf << "parsing record item '" << name_ << "' with index " << index << " size " << size
               << " current parsed bytes " << current_parsed_bytes
               << " total_size " << total_size << logendl;

    size_t parsed_bytes{0};

    if (debug)
        loginf << "parsing record item '" + name_ + "' field specification" << logendl;

    if (index + parsed_bytes >= total_size)
    {
        logerr << "unexpected record item at index " << index + parsed_bytes
               << " total_size " << total_size << ", quitting";
        return parsed_bytes;
    }

    auto* fspec_parser = static_cast<ExtendableBitsItemParser*>(field_specification_.get());
    std::vector<bool> fspec_bits;
    parsed_bytes = fspec_parser->parseItemBits(data, index + parsed_bytes, size, parsed_bytes,
                                               total_size, fspec_bits, debug);

    if (fspec_bits.size() > max_fspec_bits_)
        throw runtime_error("record item '" + name_ +
                            "' has more FSPEC bits ("+to_string(fspec_bits.size())
                            +") than defined uap items ("+to_string(max_fspec_bits_)+")");

    if (debug)
        loginf << "parsing record item '" + name_ + "' data items" << logendl;

    size_t uap_cnt{0};
    size_t num_fspec_bits = fspec_bits.size();

    bool special_purpose_field_present{false};
    bool reserved_expansion_field_present{false};

    size_t uap_size = uap_items_.size();
    for (; uap_cnt < num_fspec_bits && uap_cnt < uap_size; ++uap_cnt)
    {
        if (debug)
            loginf << " item name '" + uap_names_[uap_cnt] + "'" << logendl;

        if (!fspec_bits[uap_cnt])
            continue;

        ItemParserBase* item_parser = uap_items_[uap_cnt];
        if (!item_parser)
        {
            // FX, -, SP, or RE entry
            const auto& item_name = uap_names_[uap_cnt];
            if (item_name == "SP")
                special_purpose_field_present = true;
            else if (item_name == "RE")
                reserved_expansion_field_present = true;
            continue;
        }

        if (debug)
            loginf << "parsing record item '" << name_ << "' data item '" << uap_names_[uap_cnt]
                   << "' index " << index + parsed_bytes << logendl;

        if (index + parsed_bytes >= total_size)
        {
            logerr << "unexpected record item at index " << index + parsed_bytes
                   << " total_size " << total_size << ", quitting";
            return parsed_bytes;
        }

        parsed_bytes += item_parser->parseItem(data, index + parsed_bytes, size,
                                               parsed_bytes, total_size, target, debug);

        if (parsed_bytes > size)
        {
            logerr << "record '" << name_ << "' parsed size (" << parsed_bytes
                   << ") overrun data block size (" << size << ")" << logendl;
            break;
        }
    }

    if (has_conditional_uap_)
    {
        if (debug)
            loginf << "parsing record item '" + name_ + "' conditional uap data items" << logendl;

        std::string json_value = getValue(target, debug);

        if (debug)
            loginf << "conditional uap data item value '" << json_value << "'" << logendl;

        if (!conditional_uap_names_.count(json_value))
        {
            if (debug)
                loginf << "conditional uap data item value '" << json_value << "' not defined" << logendl;

            throw runtime_error("conditional uap data item value '" + json_value + "' not defined");
        }

        if (debug)
            loginf << "record item '" << name_ << "' with conditional " << conditional_uaps_key_ << " value "
                   << json_value << " found" << logendl;

        const auto& current_uap_names = conditional_uap_names_.at(json_value);
        const auto& current_uap_items = conditional_uap_items_.at(json_value);
        size_t cond_uap_size = current_uap_items.size();

        for (size_t cond_idx = 0; uap_cnt < num_fspec_bits && cond_idx < cond_uap_size; ++uap_cnt, ++cond_idx)
        {
            if (debug)
            {
                loginf << " conditional item name '" + current_uap_names[cond_idx] + "'" << logendl;

                if (uap_cnt >= num_fspec_bits)
                {
                    loginf << "conditional uap data item count break, uap_cnt " << uap_cnt
                           << " num_fspec_bits " << num_fspec_bits << logendl;
                    break;
                }
            }

            if (!fspec_bits[uap_cnt])
                continue;

            ItemParserBase* item_parser = current_uap_items[cond_idx];
            if (!item_parser)
            {
                // FX, -, SP, or RE entry
                const auto& item_name = current_uap_names[cond_idx];
                if (item_name == "SP")
                    special_purpose_field_present = true;
                else if (item_name == "RE")
                    reserved_expansion_field_present = true;
                continue;
            }

            if (debug)
                loginf << "parsing record item '" << name_ << "' data item '"
                       << current_uap_names[cond_idx] << "' index " << index + parsed_bytes << logendl;

            if (index + parsed_bytes >= total_size)
            {
                logerr << "unexpected uap record item at index " << index + parsed_bytes
                       << " total_size " << total_size << ", quitting";
                return parsed_bytes;
            }

            parsed_bytes += item_parser->parseItem(
                data, index + parsed_bytes, size, parsed_bytes, total_size, target, debug);

            if (parsed_bytes > size)
            {
                logerr << "record '" << name_ << "' parsed size (" << parsed_bytes
                       << ") overrun data block size (" << size << ")" << logendl;
                break;
            }
        }
    }

    if (reserved_expansion_field_present)
    {
        size_t re_bytes = static_cast<unsigned char>(data[index + parsed_bytes]);

        if (re_bytes) // skip if empty
        {
            parsed_bytes += 1;  // read 1 len byte
            re_bytes -= 1;      // includes 1 len byte

            if (ref_)  // decode ref
            {
                if (debug)
                    loginf << "record '" + name_ + "' has reserved expansion field, reading "
                           << re_bytes << " bytes " << logendl;

                if (re_bytes == 0)
                {
                    logerr << "record '" + name_ + "' has reserved expansion field with "
                           << re_bytes << " length " << logendl;
                }
                else
                {
                    traced_assert(re_bytes >= 1);

                    if (index + parsed_bytes >= total_size)
                    {
                        logerr << "unexpected record ref item at index " << index + parsed_bytes
                               << " total_size " << total_size << ", quitting";
                        return parsed_bytes;
                    }

                    size_t ref_bytes =
                        ref_->parseItem(data, index + parsed_bytes, re_bytes, 0, total_size, target["REF"], debug);

                    if (debug)
                        loginf << "record '" + name_ + "' parsed reserved expansion field, read "
                               << ref_bytes << " ref in " << re_bytes << " bytes " << logendl;

                    if (ref_bytes != re_bytes)
                    {
                        logerr << "parsing error in REF '"
                               << binary2hex((const unsigned char*)&data[index + parsed_bytes], re_bytes) << "'";

                        throw runtime_error(
                            "record item '" + name_ + "' reserved expansion field definition read " +
                            to_string(ref_bytes) + " bytes instead of specified " + to_string(re_bytes));
                    }

                    parsed_bytes += re_bytes;
                }
                // loginf << "UGA REF '" << target["REF"].dump(4) << "'" << logendl;
            }
            else
            {
                if (debug)
                    loginf << "record '" + name_ + "' has reserved expansion field, reading "
                           << re_bytes << " bytes " << logendl;

                if (index + parsed_bytes + re_bytes > total_size)
                    throw std::runtime_error("reserved expansion field longer than max size");

                if (index + parsed_bytes >= total_size)
                {
                    logerr << "unexpected record item ref at index " << index + parsed_bytes
                           << " total_size " << total_size << ", quitting";
                    return parsed_bytes;
                }

                target["REF"] = binary2hex((const unsigned char*)&data[index + parsed_bytes], re_bytes);

                parsed_bytes += re_bytes;
            }
        }
    }

    if (special_purpose_field_present)
    {
        size_t re_bytes = static_cast<unsigned char>(data[index + parsed_bytes]);

        if (re_bytes) // skip if empty
        {
            parsed_bytes += 1;  // read 1 len byte
            re_bytes -= 1;      // includes 1 len byte

            if (spf_)  // decode ref
            {
                if (debug)
                    loginf << "record '" + name_ + "' has special purpose field, reading " << re_bytes
                           << " bytes " << logendl;

                if (re_bytes == 0)
                {
                    logerr << "record '" + name_ + "' has special purpose field with "
                           << re_bytes << " length " << logendl;
                }
                else
                {
                    traced_assert(re_bytes >= 1);

                    if (index + parsed_bytes >= total_size)
                    {
                        logerr << "unexpected record spf item at index " << index + parsed_bytes
                               << " total_size " << total_size << ", quitting";
                        return parsed_bytes;
                    }

                    size_t ref_bytes = spf_->parseItem(
                        data, index + parsed_bytes, re_bytes, 0, total_size, target["SPF"], debug);

                    if (debug)
                        loginf << "record '" + name_ + "' parsed special purpose field, read " << ref_bytes
                               << " ref in " << re_bytes << " bytes " << logendl;

                    if (ref_bytes != re_bytes)
                    {
                        logerr << "parsing error in SPF '"
                               << binary2hex((const unsigned char*)&data[index + parsed_bytes], re_bytes) << "'";

                        throw runtime_error(
                            "record item '" + name_ + "' special purpose field definition read " +
                            to_string(ref_bytes) + " bytes instead of specified " + to_string(re_bytes));
                    }

                    parsed_bytes += re_bytes;
                }
                // loginf << "UGA SPF '" << target["SPF"].dump(4) << "'" << logendl;
            }
            else
            {
                if (debug)
                    loginf << "record '" + name_ + "' has special purpose field, reading " << re_bytes
                           << " bytes " << logendl;

                if (index + parsed_bytes + re_bytes > total_size)
                    throw std::runtime_error("special purpose field longer than max size");

                if (index + parsed_bytes >= total_size)
                {
                    logerr << "unexpected record spf item at index " << index + parsed_bytes
                           << " total_size " << total_size << ", quitting";
                    return parsed_bytes;
                }

                target["SPF"] = binary2hex((const unsigned char*)&data[index + parsed_bytes], re_bytes);
                parsed_bytes += re_bytes;
            }
        }
    }

    return parsed_bytes;
}

bool Record::compareKey(const nlohmann::json& container, const std::string& value, bool debug)
{
    const nlohmann::json* val_ptr = &container;

    for (const std::string& sub_key : conditional_uaps_sub_keys_)
    {
        if (debug)
            loginf << "Record: compareKey: sub_key '" << sub_key << "' json '" << val_ptr->dump(4) << "'"<< logendl;

        val_ptr = &(*val_ptr)[sub_key];
    }

    if (debug)
        loginf << "Record: compareKey: last json '" << val_ptr->dump(4) << "'"<< logendl;

    bool ret {false};
    std::string json_value;

    if (val_ptr->type() == nlohmann::json::value_t::string)  // from string conv
        json_value = val_ptr->get<std::string>();
    else
        json_value = val_ptr->dump();

    ret = json_value == value;

    if (debug)
        loginf << "Record: compareKey: json value '" << json_value << "' value '" << value << "' ret " << ret << logendl;

    return ret;
}

std::string Record::getValue(const nlohmann::json& container, bool debug)
{
    const nlohmann::json* val_ptr = &container;

    if (column_mode_)
    {
        // In column mode, containers skip nesting — leaves write flat to target.
        // Look up the last sub-key directly (the leaf name).
        const auto& leaf_key = conditional_uaps_sub_keys_.back();

        if (debug)
            loginf << "Record: getValue (column_mode): leaf_key '" << leaf_key << "'" << logendl;

        if (!container.contains(leaf_key))
            return "";

        val_ptr = &container.at(leaf_key);
    }
    else
    {
        for (const std::string& sub_key : conditional_uaps_sub_keys_)
        {
            if (debug)
                loginf << "Record: getValue: sub_key '" << sub_key << "' json '" << val_ptr->dump(4) << "'" << logendl;

            if (!val_ptr->contains(sub_key))
            {
                if (debug)
                    loginf << "Record: getValue: sub_key '" << sub_key << "' not found" << logendl;

                return "";
            }

            val_ptr = &(*val_ptr)[sub_key];
        }
    }

    std::string json_value;

    if (val_ptr->type() == nlohmann::json::value_t::string)  // from string conv
        json_value = val_ptr->get<std::string>();
    else
        json_value = val_ptr->dump();

    if (debug)
        loginf << "Record: compareKey: json value '" << json_value << "'" << logendl;

    return json_value;
}

bool Record::decodeREF() const { return decode_ref_; }

void Record::decodeREF(bool decode_ref) { decode_ref_ = decode_ref; }

std::shared_ptr<ReservedExpansionField> Record::ref() const { return ref_; }

void Record::setRef(const std::shared_ptr<ReservedExpansionField>& ref) { ref_ = ref; }

std::shared_ptr<SpecialPurposeField> Record::spf() const { return spf_; }

void Record::setSpf(const std::shared_ptr<SpecialPurposeField>& spf) { spf_ = spf; }

size_t Record::encodeItem(const nlohmann::json& source, char* target,
                          size_t max_size, bool debug)
{
    return encodeRecord(source, target, max_size, debug);
}

size_t Record::encodeRecord(const nlohmann::json& record_json, char* target,
                            size_t max_size, bool debug)
{
    if (debug)
        loginf << "encoding record '" << name_ << "'" << logendl;

    // Reconstruct FSPEC from which data items are present in the JSON
    // First pass: determine which UAP positions have data
    std::vector<bool> fspec_bits;

    // Collect all UAP names (base + conditional)
    std::vector<const std::string*> all_uap_names;
    std::vector<ItemParserBase*> all_uap_items;

    for (size_t i = 0; i < uap_names_.size(); ++i)
    {
        all_uap_names.push_back(&uap_names_[i]);
        all_uap_items.push_back(uap_items_[i]);
    }

    const std::vector<std::string>* cond_names_ptr = nullptr;
    const std::vector<ItemParserBase*>* cond_items_ptr = nullptr;

    if (has_conditional_uap_)
    {
        std::string json_value = getValue(record_json, debug);
        if (conditional_uap_names_.count(json_value))
        {
            cond_names_ptr = &conditional_uap_names_.at(json_value);
            cond_items_ptr = &conditional_uap_items_.at(json_value);
            for (size_t i = 0; i < cond_names_ptr->size(); ++i)
            {
                all_uap_names.push_back(&(*cond_names_ptr)[i]);
                all_uap_items.push_back((*cond_items_ptr)[i]);
            }
        }
    }

    // Build raw FSPEC bits (without FX bits — those are at positions 7, 15, 23...)
    size_t total_uap = all_uap_names.size();
    fspec_bits.resize(total_uap, false);

    for (size_t i = 0; i < total_uap; ++i)
    {
        const std::string& item_name = *all_uap_names[i];
        if (item_name == "FX")
            continue;  // FX bits computed below
        else if (item_name == "-")
            continue;  // spare bit
        else if (item_name == "SP")
            fspec_bits[i] = record_json.contains("SPF");
        else if (item_name == "RE")
            fspec_bits[i] = record_json.contains("REF");
        else
            fspec_bits[i] = (all_uap_items[i] != nullptr && record_json.contains(item_name));
    }

    // Determine how many FSPEC bytes needed and set FX bits
    // FX bit is at position 7, 15, 23... (last bit of each 8-bit group)
    // Pad to full bytes
    size_t num_bytes = (total_uap + 7) / 8;
    fspec_bits.resize(num_bytes * 8, false);

    // Find last byte that has any data bit set
    size_t last_needed_byte = 0;
    for (size_t byte_idx = 0; byte_idx < num_bytes; ++byte_idx)
    {
        for (size_t bit = 0; bit < 7; ++bit)  // skip FX position (bit 7 of group)
        {
            if (fspec_bits[byte_idx * 8 + bit])
            {
                last_needed_byte = byte_idx;
                break;
            }
        }
    }

    // Set FX bits: byte N's FX = 1 if we need byte N+1
    for (size_t byte_idx = 0; byte_idx < last_needed_byte; ++byte_idx)
        fspec_bits[byte_idx * 8 + 7] = true;   // FX = 1 (extend)
    // last_needed_byte's FX = 0 (no more extension)
    fspec_bits[last_needed_byte * 8 + 7] = false;

    // Trim to actual size
    fspec_bits.resize((last_needed_byte + 1) * 8);

    // Encode FSPEC
    auto* fspec_parser = static_cast<ExtendableBitsItemParser*>(field_specification_.get());
    size_t written_bytes = fspec_parser->encodeBits(fspec_bits, target, max_size, debug);

    if (debug)
        loginf << "encoding record '" << name_ << "' FSPEC " << written_bytes << " bytes" << logendl;

    // Encode data items
    size_t uap_cnt{0};
    size_t num_fspec_bits_val = fspec_bits.size();

    bool special_purpose_field_present{false};
    bool reserved_expansion_field_present{false};

    size_t uap_size = uap_items_.size();
    for (; uap_cnt < num_fspec_bits_val && uap_cnt < uap_size; ++uap_cnt)
    {
        if (!fspec_bits[uap_cnt])
            continue;

        ItemParserBase* item_parser = uap_items_[uap_cnt];
        if (!item_parser)
        {
            const auto& item_name = uap_names_[uap_cnt];
            if (item_name == "SP")
                special_purpose_field_present = true;
            else if (item_name == "RE")
                reserved_expansion_field_present = true;
            continue;
        }

        if (debug)
            loginf << "encoding record '" << name_ << "' data item '" << uap_names_[uap_cnt]
                   << "'" << logendl;

        written_bytes += item_parser->encodeItem(record_json, target + written_bytes,
                                                 max_size - written_bytes, debug);
    }

    // Conditional UAP
    if (has_conditional_uap_ && cond_names_ptr)
    {
        size_t cond_uap_size = cond_items_ptr->size();

        for (size_t cond_idx = 0; uap_cnt < num_fspec_bits_val && cond_idx < cond_uap_size;
             ++uap_cnt, ++cond_idx)
        {
            if (!fspec_bits[uap_cnt])
                continue;

            ItemParserBase* item_parser = (*cond_items_ptr)[cond_idx];
            if (!item_parser)
            {
                const auto& item_name = (*cond_names_ptr)[cond_idx];
                if (item_name == "SP")
                    special_purpose_field_present = true;
                else if (item_name == "RE")
                    reserved_expansion_field_present = true;
                continue;
            }

            if (debug)
                loginf << "encoding record '" << name_ << "' conditional data item '"
                       << (*cond_names_ptr)[cond_idx] << "'" << logendl;

            written_bytes += item_parser->encodeItem(record_json, target + written_bytes,
                                                     max_size - written_bytes, debug);
        }
    }

    // Reserved Expansion Field
    if (reserved_expansion_field_present && record_json.contains("REF"))
    {
        const json& ref_json = record_json.at("REF");

        if (ref_ && ref_json.is_object())
        {
            // encode REF content into a temp buffer to measure length
            char ref_buf[4096];
            size_t ref_bytes = ref_->encodeItem(ref_json, ref_buf, sizeof(ref_buf), debug);

            // write length byte (includes itself)
            target[written_bytes] = static_cast<char>(ref_bytes + 1);
            written_bytes += 1;

            // copy REF content
            memcpy(target + written_bytes, ref_buf, ref_bytes);
            written_bytes += ref_bytes;
        }
        else if (ref_json.is_string())
        {
            // hex string fallback
            std::string hex_str = ref_json.get<std::string>();
            size_t ref_bytes = hex_str.size() / 2;

            target[written_bytes] = static_cast<char>(ref_bytes + 1);
            written_bytes += 1;

            hex2bin(hex_str.c_str(), target + written_bytes);
            written_bytes += ref_bytes;
        }
    }

    // Special Purpose Field
    if (special_purpose_field_present && record_json.contains("SPF"))
    {
        const json& spf_json = record_json.at("SPF");

        if (spf_ && spf_json.is_object())
        {
            char spf_buf[4096];
            size_t spf_bytes = spf_->encodeItem(spf_json, spf_buf, sizeof(spf_buf), debug);

            target[written_bytes] = static_cast<char>(spf_bytes + 1);
            written_bytes += 1;

            memcpy(target + written_bytes, spf_buf, spf_bytes);
            written_bytes += spf_bytes;
        }
        else if (spf_json.is_string())
        {
            std::string hex_str = spf_json.get<std::string>();
            size_t spf_bytes = hex_str.size() / 2;

            target[written_bytes] = static_cast<char>(spf_bytes + 1);
            written_bytes += 1;

            hex2bin(hex_str.c_str(), target + written_bytes);
            written_bytes += spf_bytes;
        }
    }

    if (debug)
        loginf << "encoding record '" << name_ << "' done, " << written_bytes << " bytes"
               << logendl;

    return written_bytes;
}

void Record::addInfo (const std::string& edition, CategoryItemInfo& info) const
{
    for (auto& item_it : items_)
        item_it.second->addInfo(edition, info);
}

void Record::setupColumnWriters(const LeafSetupCallback& callback)
{
    column_mode_ = true;

    // Walk all items (covers both base UAP and conditional UAP entries)
    for (auto& [name, item] : items_)
        item->setupColumnWriters(callback);

    // REF/SPF parser trees are attached separately from the UAP items
    if (ref_)
        ref_->setupColumnWriters(callback);

    if (spf_)
        spf_->setupColumnWriters(callback);
}

// bool Record::compareKey (const nlohmann::json& container, const std::string& value)
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
