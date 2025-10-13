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

#include "edition.h"
#include "record.h"
#include "traced_assert.h"

namespace jASTERIX
{
using namespace std;
using namespace nlohmann;

Edition::Edition(const std::string& number, const nlohmann::json& definition,
                 const std::string& definition_path)
    : EditionBase(number, definition, definition_path)
{
    record_.reset(new Record(definition_));
}

Edition::~Edition() {}

std::shared_ptr<Record> Edition::record() const { return record_; }

void Edition::addInfo (const std::string& edition, CategoryItemInfo& info)
{
    traced_assert(record_);
    record_->addInfo(edition, info);
}

}  // namespace jASTERIX
