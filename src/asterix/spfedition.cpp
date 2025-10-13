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

#include "spfedition.h"
#include "ref.h"
#include "traced_assert.h"

namespace jASTERIX
{
using namespace std;
using namespace nlohmann;

SPFEdition::SPFEdition(const std::string& number, const nlohmann::json& definition,
                       const std::string& definition_path)
    : EditionBase(number, definition, definition_path)
{
    spf_.reset(new SpecialPurposeField(definition_));
}

SPFEdition::~SPFEdition() {}

std::shared_ptr<SpecialPurposeField> SPFEdition::specialPurposeField() const { return spf_; }

void SPFEdition::addInfo (const std::string& edition, CategoryItemInfo& info)
{
    traced_assert(spf_);
    spf_->addInfo(edition, info);
}

}  // namespace jASTERIX
