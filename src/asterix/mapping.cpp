/*
 * This file is part of ATSDB.
 *
 * ATSDB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ATSDB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with ATSDB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mapping.h"

#include <fstream>

#include "files.h"
#include "logger.h"
#include "string_conv.h"
#include "traced_assert.h"

namespace jASTERIX
{
using namespace Files;
using namespace std;
using namespace nlohmann;

Mapping::Mapping(const std::string& name, const nlohmann::json& definition,
                 const std::string& definition_path)
    : name_(name)
{
    //    "comment":"conversion to general format 1.0",
    if (!definition.contains("comment"))
        throw runtime_error("mapping '" + name_ + "' has no comment");

    comment_ = definition.at("comment");

    //    "file": "048/cat048_1.15.json"
    if (!definition.contains("file"))
        throw runtime_error("mapping '" + name_ + "' has no file");

    file_ = definition.at("file");

    mapping_definition_path_ = definition_path + "/categories/" + file_;

    if (!fileExists(mapping_definition_path_))
        throw invalid_argument("mapping " + name_ + " file '" + mapping_definition_path_ +
                               "' not found");

    definition_ = json::parse(ifstream(mapping_definition_path_));
}

std::string Mapping::name() const { return name_; }

std::string Mapping::comment() const { return comment_; }

std::string Mapping::file() const { return file_; }

void Mapping::map(nlohmann::json& src, nlohmann::json& dest)
{
    traced_assert(src.is_object());
    // loginf << "mapping: map";
    mapObject(definition_, src, dest);
}

std::string Mapping::definitionPath() const { return mapping_definition_path_; }

void Mapping::mapObject(nlohmann::json& object_definition, const nlohmann::json& src,
                        nlohmann::json& dest)
{
    std::string def_key;
    std::string def_value_str;
    std::string src_value_str;

    // loginf << "mapping: mapObject";

    for (auto def_it = object_definition.begin(); def_it != object_definition.end(); ++def_it)
    {
        // std::cout << def_it.key() << " | " << def_it.value() << "\n";

        def_key = def_it.key();

        // check if exists in src
        if (src.contains(def_key))
        {
            const json& src_value = src.at(def_key);

            if (src_value.is_object())
            {
                traced_assert(def_it.value().is_object());
                mapObject(def_it.value(), src_value, dest);  // iterate into sub-object
            }
            else
            {
                if (def_it.value()
                        .is_object())  // contains (src value)->(dest key,dest value) entries
                {
                    src_value_str = toString(src_value);

                    if (def_it.value().contains(src_value_str))  // skip if not defined
                    {
                        const json& src_val_mapped_obj = def_it.value()[src_value_str];
                        traced_assert(src_val_mapped_obj.is_object());

                        for (auto dest_it = src_val_mapped_obj.begin();
                             dest_it != src_val_mapped_obj.end(); ++dest_it)
                        {
                            def_value_str = dest_it.key();
                            mapKey(def_value_str, dest_it.value(), dest);
                        }
                    }
                }
                else
                {
                    def_value_str = def_it.value();

                    if (def_value_str.size())  // skip unmapped
                        mapKey(def_value_str, src_value, dest);
                }
            }
        }
    }
}

void Mapping::mapKey(std::string& key_definition, const nlohmann::json& src_value,
                     nlohmann::json& dest)
{
    // loginf << "mapping key '" << key_definition << "' src value '" << src_value << "'";

    std::vector<std::string> sub_keys;
    split(key_definition, '.', sub_keys);

    nlohmann::json* val_ptr = &dest;

    for (const std::string& sub_key : sub_keys)
    {
        val_ptr = &(*val_ptr)[sub_key];
    }

    *val_ptr = src_value;

    // loginf << "mapping key '" << key_definition << "' dest '" << dest.dump(4) << "'";
}

}  // namespace jASTERIX
