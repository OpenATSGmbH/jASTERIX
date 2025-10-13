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

#include <jasterix/edition.h>
#include <jasterix/mapping.h>
#include <jasterix/refedition.h>
#include <jasterix/spfedition.h>
#include <jasterix/iteminfo.h>

#include <string>

#include "json.hpp"

namespace jASTERIX
{
class Category
{
  public:
    Category(const std::string& number, const nlohmann::json& definition,
             const std::string& definition_path);
    virtual ~Category();

    std::string number() const;
    std::string comment() const;

    // edition stuff
    bool hasEdition(const std::string& edition_str) const;
    std::shared_ptr<Edition> edition(const std::string& edition_str);
    std::string editionPath(const std::string& edition_str) const;

    std::string defaultEdition() const;
    void setCurrentEdition(const std::string& edition_str);
    std::shared_ptr<Edition> getCurrentEdition();

    // ref stuff
    bool hasREFEdition(const std::string& edition_str) const;
    std::shared_ptr<REFEdition> refEdition(const std::string& edition_str);
    std::string refEditionPath(const std::string& edition_str) const;

    std::string defaultREFEdition() const;
    void setCurrentREFEdition(const std::string& edition_str);
    bool hasCurrentREFEdition();
    std::shared_ptr<REFEdition> getCurrentREFEdition();

    // spf stuff
    bool hasSPFEdition(const std::string& edition_str) const;
    std::shared_ptr<SPFEdition> spfEdition(const std::string& edition_str);
    std::string spfEditionPath(const std::string& edition_str) const;

    std::string defaultSPFEdition() const;
    void setCurrentSPFEdition(const std::string& edition_str);
    bool hasCurrentSPFEdition();
    std::shared_ptr<SPFEdition> getCurrentSPFEdition();

    // mapping stuff
    bool hasMapping(const std::string& mapping_str);
    std::shared_ptr<Mapping> mapping(const std::string& mapping_str);

    std::string defaultMapping() const;
    bool hasCurrentMapping();
    void setCurrentMapping(const std::string& mapping_str);
    std::shared_ptr<Mapping> getCurrentMapping();

    const std::map<std::string, std::shared_ptr<Edition>>& editions() const;
    const std::map<std::string, std::shared_ptr<REFEdition>>& refEditions() const;
    const std::map<std::string, std::shared_ptr<SPFEdition>>& spfEditions() const;
    const std::map<std::string, std::shared_ptr<Mapping>>& mappings() const;

    bool decode() const;
    void decode(bool value);

    CategoryItemInfo itemInfo () const;


  protected:
    std::string number_;
    std::string comment_;

    std::string default_edition_;
    std::string default_ref_edition_;
    std::string default_spf_edition_;
    std::string default_mapping_;

    std::string current_edition_;
    std::string current_ref_edition_;
    std::string current_spf_edition_;
    std::string current_mapping_;

    bool decode_{true};

    std::map<std::string, std::shared_ptr<Edition>> editions_;
    std::map<std::string, std::shared_ptr<REFEdition>> ref_editions_;
    std::map<std::string, std::shared_ptr<SPFEdition>> spf_editions_;
    std::map<std::string, std::shared_ptr<Mapping>> mappings_;
};

}  // namespace jASTERIX
