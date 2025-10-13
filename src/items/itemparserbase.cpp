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

#include <cassert>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>
#include <algorithm>

#include "compounditemparser.h"
#include "dynamicbytesitemparser.h"
#include "extendablebitsitemparser.h"
#include "extendableitemparser.h"
#include "fixedbitfielditemparser.h"
#include "fixedbitsitemparser.h"
#include "fixedbytesitemparser.h"
#include "itemparser.h"
#include "logger.h"
#include "optionalitemparser.h"
#include "repetetiveitemparser.h"
#include "skipbytesitemparser.h"

using namespace std;
using namespace nlohmann;

namespace jASTERIX
{
ItemParserBase::ItemParserBase(const nlohmann::json& item_definition, const std::string& long_name_prefix)
    : item_definition_(item_definition), long_name_prefix_(long_name_prefix)
{
    if (!item_definition.contains("name"))
        throw runtime_error("item construction without JSON name definition");

    name_ = item_definition.at("name");

    if (!item_definition.contains("type"))
        throw runtime_error("item '" + name_ + "' construction without data type definition");

    type_ = item_definition.at("type");

    if (long_name_prefix_.size())
        long_name_ = long_name_prefix_ + "." + name_;
    else
        long_name_ = name_;

    //loginf << "UGA name '" << name_ << "' long '" << long_name_ << "'" << logendl;
}

ItemParserBase* ItemParserBase::createItemParser(const nlohmann::json& item_definition,
                                                 const std::string& long_name_prefix)
{
    if (!item_definition.contains("name"))
        throw runtime_error("item creation without JSON name definition");

    std::string name = item_definition.at("name");

    if (!item_definition.contains("type"))
        throw runtime_error("item '" + name + "' creation without data type definition");

    std::string type = item_definition.at("type");

    if (type == "item")
    {
        return new ItemParser(item_definition, long_name_prefix);
    }
    else if (type == "fixed_bytes")
    {
        return new FixedBytesItemParser(item_definition, long_name_prefix);
    }
    else if (type == "skip_bytes")
    {
        return new SkipBytesItemParser(item_definition, long_name_prefix);
    }
    else if (type == "dynamic_bytes")
    {
        return new DynamicBytesItemParser(item_definition, long_name_prefix);
    }
    else if (type == "compound")
    {
        return new CompoundItemParser(item_definition, long_name_prefix);
    }
    else if (type == "extendable_bits")
    {
        return new ExtendableBitsItemParser(item_definition, long_name_prefix);
    }
    else if (type == "extendable")
    {
        return new ExtendableItemParser(item_definition, long_name_prefix);
    }
    else if (type == "fixed_bitfield")
    {
        return new FixedBitFieldItemParser(item_definition, long_name_prefix);
    }
    else if (type == "optional_item")
    {
        return new OptionalItemParser(item_definition, long_name_prefix);
    }
    else if (type == "repetitive")
    {
        return new RepetetiveItemParser(item_definition, long_name_prefix);
    }
    else
        throw runtime_error("item creation name '" + name + "' with unknown type '" + type + "'");
}

std::string ItemParserBase::name() const { return name_; }

std::string ItemParserBase::type() const { return type_; }

void ItemParserBase::addInfo (const std::string& edition, CategoryItemInfo& info) const
{
    string comment;

    if (item_definition_.contains("comment"))
    {
        comment = item_definition_.at("comment");
    }

    if (!info.count(long_name_))
        info[long_name_].description_ = comment;

    info.at(long_name_).editions_.insert(edition);

    // TODO edition

}

std::string ItemParserBase::longName() const
{
    return long_name_;
}

std::string ItemParserBase::longNamePrefix() const
{
    return long_name_prefix_;
}

// size_t parseFixedBitsItem (const std::string& name, const std::string& type, const
// nlohmann::json& item_definition,
//                           const char* data, size_t index, size_t size, size_t
//                           current_parsed_bytes, nlohmann::json& target, nlohmann::json& parent,
//                           bool debug)
//{
//    if (debug)
//    {
//        traced_assert(type == "fixed_bits");
//        loginf << "parsing fixed bits item '" << name << "'";
//    }

//    if (debug && item_definition.find("start_bit") == item_definition.end())
//        throw runtime_error ("parsing fixed byte bitfield item '"+name+"' without start bit");

//    unsigned int start_bit = item_definition.at("start_bit");

//    if (debug && item_definition.find("bit_length") == item_definition.end())
//        throw runtime_error ("parsing fixed byte bitfield item '"+name+"' without bit length");

//    unsigned int bit_length = item_definition.at("bit_length");
//    unsigned int byte_length = size;

//    size_t tmp_data{0};

//    const char* current_data = &data[index];

//    if (byte_length == 1)
//        tmp_data = *reinterpret_cast<const unsigned char*> (&current_data[0]);
//    else
//    {
//        unsigned char tmp;
//        for (size_t cnt = 0; cnt < byte_length; ++cnt)
//        {
//            tmp = *reinterpret_cast<const unsigned char*> (&current_data[cnt]);

//            if (debug)
//                loginf << "fixed byte bitfield item '"+name+"' cnt " << cnt << " byte "
//                       << std::hex << static_cast<unsigned int> (tmp) << " data " << tmp_data;

//            tmp_data = (tmp_data << 8) + tmp;
//        }
//    }

//    if (debug)
//        loginf << "parsing fixed bits item '" << name << "'"
//               << " byte length " << byte_length
//               << " current data '" << hex << tmp_data << "'"
//               << " with start bit " << start_bit << " length " << bit_length;

//    size_t bitmask {1};
//    bitmask <<= start_bit+bit_length-1;

//    bool bit_set {false};
//    size_t value {0};

//    for (unsigned cnt=0; cnt < bit_length; ++cnt)
//    {
//        value <<= 1;
//        bit_set = tmp_data & bitmask;
//        value |= bit_set;

//        if (debug)
//            loginf << "parsing fixed bits item '" << name << "' with bit " << cnt
//                   << " bitmask " << bitmask << " set " << bit_set << " value " << value;

//        bitmask >>= 1;
//    }

//    if (debug)
//        loginf << "parsing fixed bits item '" << name << "' with start bit " << start_bit
//               << " length " << bit_length << " value " << value;

//    target = value;

//    return 0;
//}

// bool variableHasValue (const nlohmann::json& data, const std::string& variable_name,
//                       const nlohmann::json& variable_value)
//{
//    const nlohmann::json* val_ptr = &data;
//    std::vector <std::string> sub_keys = split(variable_name, '.');
//    for (const std::string& sub_key : sub_keys)
//    {
//        if (val_ptr->contains (sub_key))
//        {
//            if (sub_key == sub_keys.back()) // last found
//            {
//                val_ptr = &val_ptr->at(sub_key);
//                break;
//            }

//            if (val_ptr->at(sub_key).is_object()) // not last, step in
//                val_ptr = &val_ptr->at(sub_key);
//            else // not last key, and not object
//            {
//                val_ptr = nullptr;
//                break;
//            }
//        }
//        else // not found
//        {
//            val_ptr = nullptr;
//            break;
//        }
//    }

//    if (val_ptr == nullptr || *val_ptr == nullptr) // not found
//        return false;
//    else
//        return *val_ptr == variable_value;
//}

bool variableHasValue(const nlohmann::json& data,
                      const std::vector<std::string>& variable_name_parts,
                      const nlohmann::json& variable_value)
{
    const nlohmann::json* val_ptr = &data;
    // std::vector <std::string> sub_keys = split(variable_name, '.');
    for (const std::string& sub_key : variable_name_parts)
    {
        if (val_ptr->contains(sub_key))
        {
            if (sub_key == variable_name_parts.back())  // last found
            {
                val_ptr = &val_ptr->at(sub_key);
                break;
            }

            if (val_ptr->at(sub_key).is_object())  // not last, step in
                val_ptr = &val_ptr->at(sub_key);
            else  // not last key, and not object
            {
                val_ptr = nullptr;
                break;
            }
        }
        else  // not found
        {
            val_ptr = nullptr;
            break;
        }
    }

    if (val_ptr == nullptr || *val_ptr == nullptr)  // not found
        return false;
    else
        return *val_ptr == variable_value;
}

}  // namespace jASTERIX
