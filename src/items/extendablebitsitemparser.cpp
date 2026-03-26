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

#include "extendablebitsitemparser.h"

#include <algorithm>

#include "logger.h"
#include "traced_assert.h"

using namespace std;
using namespace nlohmann;

namespace jASTERIX
{
ExtendableBitsItemParser::ExtendableBitsItemParser(const nlohmann::json& item_definition,
                                                   const std::string& long_name_prefix)
    : ItemParserBase(item_definition, long_name_prefix)
{
    traced_assert(type_ == "extendable_bits");

    if (!item_definition.contains("data_type"))
        throw runtime_error("extendable bits item '" + name_ + "' parsing without data type");

    data_type_ = item_definition.at("data_type");

    reverse_bits_ =
        (item_definition.contains("reverse_bits") && item_definition.at("reverse_bits") == true);

    reverse_order_ =
        (item_definition.contains("reverse_order") && item_definition.at("reverse_order") == true);
}

size_t ExtendableBitsItemParser::parseItem(const char* data, size_t index, size_t size,
                                           size_t current_parsed_bytes, size_t total_size,
                                           nlohmann::json& target, bool debug)
{
    if (debug)
        loginf << "parsing extendable bits item '" << name_ << "' index " << index << " size "
               << size << " current parsed bytes " << current_parsed_bytes << logendl;

    const char* current_data = &data[index];

    if (data_type_ == "bitfield")
    {
        unsigned int bitmask;
        std::vector<bool> bitfield;
        bool value = true;
        size_t parsed_bytes{0};

        while (value != false)  // last value is extension bit
        {
            if (index + parsed_bytes >= total_size)
                throw runtime_error("ExtendableBitsItemParser '" + name_ + "': FX loop at index " +
                    to_string(index + parsed_bytes) + " exceeds total_size " + to_string(total_size));

            const unsigned char current_byte =
                *reinterpret_cast<const unsigned char*>(&current_data[parsed_bytes]);

            if (reverse_bits_)
            {
                bitmask = 1;
                bitmask <<= 7;

                for (size_t cnt{0}; cnt < 8; ++cnt)
                {
                    value = current_byte & bitmask;
                    bitfield.push_back(value);

                    if (debug)
                        loginf << "extendable bits item '" << name_ << "' index "
                               << index + parsed_bytes << " current byte "
                               << static_cast<unsigned int>(current_byte) << " reverse true "
                               << " bit field index " << cnt << " bitmask " << bitmask << " value "
                               << value << logendl;

                    bitmask >>= 1;
                }
            }
            else
            {
                bitmask = 1;

                for (size_t cnt{0}; cnt < 8; ++cnt)
                {
                    value = current_byte & bitmask;
                    bitfield.push_back(value);

                    if (debug)
                        loginf << "extendable bits item '" << name_ << "' index "
                               << index + parsed_bytes << " current byte "
                               << static_cast<unsigned int>(current_byte) << " reverse false "
                               << " bit field index " << cnt << " bitmask " << bitmask << " value "
                               << value << logendl;

                    bitmask = bitmask << 1;
                }
            }
            ++parsed_bytes;
        }

        if (reverse_order_)
            std::reverse(bitfield.begin(), bitfield.end());

        target.emplace(name_, std::move(bitfield));

        if (debug)
            loginf << "extendable bits item '" + name_ + "'"
                   << " index " << index << " parsed " << parsed_bytes << " bytes" << logendl;

        return parsed_bytes;
    }
    else
        throw runtime_error("extentable bits item '" + name_ +
                            "' parsing with unknown data type '" + data_type_ + "'");
}

size_t ExtendableBitsItemParser::parseItemBits(const char* data, size_t index, size_t size,
                                               size_t current_parsed_bytes, size_t total_size,
                                               std::vector<bool>& out_bits, bool debug,
                                               const std::vector<std::string>* items_names)
{
    if (debug)
        loginf << "parsing extendable bits item '" << name_ << "' (to local) index " << index
               << " size " << size << " current parsed bytes " << current_parsed_bytes << logendl;

    const char* current_data = &data[index];

    if (data_type_ != "bitfield")
        throw runtime_error("extentable bits item '" + name_ +
                            "' parsing with unknown data type '" + data_type_ + "'");

    unsigned int bitmask;
    out_bits.clear();
    bool value = true;
    size_t parsed_bytes{0};

    while (value != false)
    {
        if (index + parsed_bytes >= total_size)
            throw runtime_error("ExtendableBitsItemParser '" + name_ + "': FX loop at index " +
                to_string(index + parsed_bytes) + " exceeds total_size " + to_string(total_size));

        const unsigned char current_byte =
            *reinterpret_cast<const unsigned char*>(&current_data[parsed_bytes]);

        if (reverse_bits_)
        {
            bitmask = 1;
            bitmask <<= 7;

            for (size_t cnt{0}; cnt < 8; ++cnt)
            {
                value = current_byte & bitmask;
                out_bits.push_back(value);
                bitmask >>= 1;
            }
        }
        else
        {
            bitmask = 1;

            for (size_t cnt{0}; cnt < 8; ++cnt)
            {
                value = current_byte & bitmask;
                out_bits.push_back(value);
                bitmask = bitmask << 1;
            }
        }
        ++parsed_bytes;

        // only extend if the last bit of this byte is an FX entry in items_names
        if (items_names)
        {
            size_t fx_index = parsed_bytes * 8 - 1;  // last bit position of current byte
            if (fx_index >= items_names->size() ||
                items_names->at(fx_index).substr(0, 2) != "FX")
                break;
        }
    }

    if (reverse_order_)
        std::reverse(out_bits.begin(), out_bits.end());

    if (debug)
        loginf << "extendable bits item '" + name_ + "' (to local)"
               << " index " << index << " parsed " << parsed_bytes << " bytes" << logendl;

    return parsed_bytes;
}

size_t ExtendableBitsItemParser::encodeBits(const std::vector<bool>& bits, char* target,
                                            size_t max_size, bool debug)
{
    if (debug)
        loginf << "encoding extendable bits item '" << name_ << "' (from local)" << logendl;

    if (data_type_ != "bitfield")
        throw runtime_error("extendable bits item '" + name_ +
                            "' encoding with unknown data type '" + data_type_ + "'");

    std::vector<bool> bitfield = bits;

    if (reverse_order_)
        std::reverse(bitfield.begin(), bitfield.end());

    size_t num_bytes = bitfield.size() / 8;
    traced_assert(bitfield.size() % 8 == 0);

    for (size_t byte_idx = 0; byte_idx < num_bytes; ++byte_idx)
    {
        unsigned char current_byte = 0;

        if (reverse_bits_)
        {
            unsigned int bitmask = 1 << 7;
            for (size_t bit = 0; bit < 8; ++bit)
            {
                if (bitfield[byte_idx * 8 + bit])
                    current_byte |= bitmask;
                bitmask >>= 1;
            }
        }
        else
        {
            unsigned int bitmask = 1;
            for (size_t bit = 0; bit < 8; ++bit)
            {
                if (bitfield[byte_idx * 8 + bit])
                    current_byte |= bitmask;
                bitmask <<= 1;
            }
        }

        target[byte_idx] = static_cast<char>(current_byte);
    }

    return num_bytes;
}

size_t ExtendableBitsItemParser::encodeItem(const nlohmann::json& source, char* target,
                                            size_t max_size, bool debug)
{
    if (debug)
        loginf << "encoding extendable bits item '" << name_ << "'" << logendl;

    if (data_type_ != "bitfield")
        throw runtime_error("extendable bits item '" + name_ +
                            "' encoding with unknown data type '" + data_type_ + "'");

    std::vector<bool> bitfield = source.at(name_).get<std::vector<bool>>();

    return encodeBits(bitfield, target, max_size, debug);
}

}  // namespace jASTERIX
