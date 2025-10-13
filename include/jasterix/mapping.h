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

#include <string>

#include "json.hpp"

namespace jASTERIX
{
class Mapping
{
  public:
    Mapping(const std::string& name, const nlohmann::json& definition,
            const std::string& definition_path);
    virtual ~Mapping() {}

    std::string name() const;
    std::string comment() const;
    std::string file() const;

    void map(nlohmann::json& src, nlohmann::json& dest);

    std::string definitionPath() const;

  protected:
    std::string name_;
    std::string comment_;
    std::string file_;

    std::string mapping_definition_path_;
    nlohmann::json definition_;  // from file

    void mapObject(nlohmann::json& object_definition, const nlohmann::json& src,
                   nlohmann::json& dest);
    void mapKey(std::string& key_definition, const nlohmann::json& src_value, nlohmann::json& dest);
};

}  // namespace jASTERIX

