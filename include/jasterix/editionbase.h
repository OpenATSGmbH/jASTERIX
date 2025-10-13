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

#include <jasterix/iteminfo.h>

#include "json.hpp"

namespace jASTERIX {


class EditionBase
{
public:
    EditionBase(const std::string& number, const nlohmann::json& definition,
                const std::string& definition_path);
    virtual ~EditionBase();

    std::string number() const;
    std::string document() const;
    std::string date() const;
    std::string file() const;
    std::string definitionPath() const;

    virtual void addInfo (const std::string& edition, CategoryItemInfo& info)=0;

protected:
    std::string number_;
    std::string document_;
    std::string date_;
    std::string file_;

    std::string edition_definition_path_;
    nlohmann::json definition_;  // from file
};

} // namespace jASTERIX
