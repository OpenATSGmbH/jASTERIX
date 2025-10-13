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

#include "refedition.h"

#include <fstream>

#include "files.h"
#include "ref.h"
#include "traced_assert.h"

namespace jASTERIX
{
using namespace Files;
using namespace std;
using namespace nlohmann;

REFEdition::REFEdition(const std::string& number, const nlohmann::json& definition,
                       const std::string& definition_path)
    : EditionBase(number, definition, definition_path)
{
    ref_.reset(new ReservedExpansionField(definition_));
}

REFEdition::~REFEdition() {}

std::shared_ptr<ReservedExpansionField> REFEdition::reservedExpansionField() const { return ref_; }

void REFEdition::addInfo (const std::string& edition, CategoryItemInfo& info)
{
    traced_assert(ref_);
    ref_->addInfo(edition, info);
}

}  // namespace jASTERIX
