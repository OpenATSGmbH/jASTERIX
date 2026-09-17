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

#include "compounditemparser.h"
#include "extendablebitsitemparser.h"
#include "optionalitemparser.h"
#include "logger.h"
#include "traced_assert.h"

using namespace std;
using namespace nlohmann;

namespace jASTERIX
{
CompoundItemParser::CompoundItemParser(const nlohmann::json& item_definition, const std::string& long_name_prefix)
    : ItemParserBase(item_definition, long_name_prefix)
{
    traced_assert(type_ == "compound");

    if (!item_definition.contains("field_specification"))
        throw runtime_error("compound item '" + name_ + "' parsing without field specification");

    const json& field_specification = item_definition.at("field_specification");

    if (!field_specification.is_object())
        throw runtime_error("parsing compound item '" + name_ +
                            "' field specification is not an object");

    // field_specification_name_ = field_specification.at("name");

    field_specification_.reset(ItemParserBase::createItemParser(field_specification, long_name_));
    traced_assert(field_specification_);

    if (!item_definition.contains("items"))
        throw runtime_error("parsing compound item '" + name_ + "' without items");

    const json& items = item_definition.at("items");

    if (!items.is_array())
        throw runtime_error("parsing compound item '" + name_ +
                            "' field specification is not an array");

    std::string item_name;
    ItemParserBase* item{nullptr};

    for (const json& data_item_it : items)
    {
        item_name = data_item_it.at("name");
        item = ItemParserBase::createItemParser(data_item_it, long_name_prefix_); // leave out own name
        traced_assert(item);
        items_.push_back(std::unique_ptr<ItemParserBase>{item});
        optional_items_.push_back(dynamic_cast<OptionalItemParser*>(item));
    }
}

size_t CompoundItemParser::parseItem(const char* data, size_t index, size_t size,
                                     size_t current_parsed_bytes, size_t total_size,
                                     nlohmann::json& target, bool debug)
{
    if (debug)
        loginf << "parsing compound item '" << name_ << "' with " << items_.size()
               << " items index " << index << " size " << size << " current parsed bytes "
               << current_parsed_bytes << logendl;

    size_t parsed_bytes{0};

    if (debug)
        loginf << "parsing compound item '" + name_ + "' field specification" << logendl;

    auto* fspec_parser = static_cast<ExtendableBitsItemParser*>(field_specification_.get());
    std::vector<bool> field_bits;
    parsed_bytes = fspec_parser->parseItemBits(
                data, index + parsed_bytes, size, parsed_bytes, total_size, field_bits, debug);

    if (debug)
        loginf << "parsing compound item '" + name_ + "' data items" << logendl;

    for (auto& data_item_it : items_)
    {
        if (debug)
            loginf << "parsing compound item '" << name_ << "' data item '" << data_item_it->name()
                   << "' index " << index + parsed_bytes << logendl;

        OptionalItemParser* opt_parser = optional_items_[&data_item_it - items_.data()];
        if (opt_parser)
        {
            parsed_bytes += opt_parser->parseItem(
                        data, index + parsed_bytes, size, parsed_bytes, total_size,
                        target, debug, field_bits);
        }
        else
        {
            parsed_bytes += data_item_it->parseItem(
                        data, index + parsed_bytes, size, parsed_bytes, total_size, target, debug);
        }

        if (index + parsed_bytes > total_size)
            throw runtime_error("CompoundItemParser '" + name_ + "': parsed " +
                to_string(parsed_bytes) + " bytes at index " + to_string(index) +
                " exceeds total_size " + to_string(total_size));
    }

    return parsed_bytes;
}

size_t CompoundItemParser::encodeItem(const nlohmann::json& source, char* target,
                                      size_t max_size, bool debug)
{
    if (debug)
        loginf << "encoding compound item '" << name_ << "'" << logendl;

    // Reconstruct field specification from which optional sub-items are present
    // Each sub-item has a bitfield_index_ that maps to the actual bit position
    size_t max_bit_index = 0;
    for (auto& data_item_it : items_)
    {
        auto* opt_parser = dynamic_cast<OptionalItemParser*>(data_item_it.get());
        if (opt_parser && opt_parser->bitfieldIndex() >= max_bit_index)
            max_bit_index = opt_parser->bitfieldIndex();
    }

    size_t num_bytes = (max_bit_index + 8) / 8;
    std::vector<bool> field_bits(num_bytes * 8, false);

    for (auto& data_item_it : items_)
    {
        auto* opt_parser = dynamic_cast<OptionalItemParser*>(data_item_it.get());
        if (opt_parser && source.contains(data_item_it->name()))
            field_bits[opt_parser->bitfieldIndex()] = true;
    }

    // Find last byte that has any data bit set
    size_t last_needed_byte = 0;
    for (size_t byte_idx = 0; byte_idx < num_bytes; ++byte_idx)
        for (size_t bit = 0; bit < 7; ++bit)
            if (field_bits[byte_idx * 8 + bit])
                last_needed_byte = byte_idx;

    // Set FX bits
    for (size_t byte_idx = 0; byte_idx < last_needed_byte; ++byte_idx)
        field_bits[byte_idx * 8 + 7] = true;
    field_bits[last_needed_byte * 8 + 7] = false;
    field_bits.resize((last_needed_byte + 1) * 8);

    size_t written_bytes{0};

    // encode field specification
    auto* fspec_parser = static_cast<ExtendableBitsItemParser*>(field_specification_.get());
    written_bytes = fspec_parser->encodeBits(field_bits, target, max_size, debug);

    // encode data items
    for (auto& data_item_it : items_)
    {
        if (debug)
            loginf << "encoding compound item '" << name_ << "' data item '"
                   << data_item_it->name() << "'" << logendl;

        auto* opt_parser = dynamic_cast<OptionalItemParser*>(data_item_it.get());
        if (opt_parser)
        {
            written_bytes += opt_parser->encodeItem(source, target + written_bytes,
                                                    max_size - written_bytes, debug, field_bits);
        }
        else
        {
            written_bytes += data_item_it->encodeItem(source, target + written_bytes,
                                                      max_size - written_bytes, debug);
        }
    }

    return written_bytes;
}

void CompoundItemParser::addInfo (const std::string& edition, CategoryItemInfo& info) const
{
    for (auto& item_it : items_)
        item_it->addInfo(edition, info);
}

void CompoundItemParser::setupColumnWriters(const LeafSetupCallback& callback)
{
    column_mode_ = true;
    for (auto& item_it : items_)
        item_it->setupColumnWriters(callback);
}

void CompoundItemParser::clearColumnWriters()
{
    ItemParserBase::clearColumnWriters();
    for (auto& item_it : items_)
        item_it->clearColumnWriters();
}

}  // namespace jASTERIX
