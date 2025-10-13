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

#include <jasterix/editionbase.h>
#include <jasterix/record.h>

#include <string>

#include "json.hpp"

namespace jASTERIX
{

class Record;

class Edition : public EditionBase
{
  public:
    Edition(const std::string& number, const nlohmann::json& definition,
            const std::string& definition_path);
    virtual ~Edition();

    std::shared_ptr<Record> record() const;

    virtual void addInfo (const std::string& edition, CategoryItemInfo& info) override;

  protected:
    std::shared_ptr<Record> record_;
};

}  // namespace jASTERIX
