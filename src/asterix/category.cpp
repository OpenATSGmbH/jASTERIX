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

#include "category.h"

#include "edition.h"
#include "logger.h"
#include "traced_assert.h"

using namespace std;
using namespace nlohmann;

namespace jASTERIX
{
Category::Category(const std::string& number, const nlohmann::json& definition,
                   const std::string& definition_path)
    : number_(number)
{
    //    "comment": "Category 048 Transmission of Monoradar Target Reports",

    if (!definition.contains("comment"))
        throw runtime_error("category '" + number_ + "' has no comment");

    comment_ = definition.at("comment");

    //    "default_edition" : "1.15",

    if (!definition.contains("default_edition"))
        throw runtime_error("category '" + number_ + "' has no default edition");

    default_edition_ = definition.at("default_edition");

    //    "default_ref_edition" : "1.8",
    if (definition.contains("default_ref_edition"))  // is optional
        default_ref_edition_ = definition.at("default_ref_edition");

    //    "default_spf_edition" : "ARTAS",
    if (definition.contains("default_spf_edition"))  // is optional
        default_spf_edition_ = definition.at("default_spf_edition");

    // decode flag
    if (definition.contains("decode"))
        decode_ = definition.at("decode");

    //    "editions":

    if (!definition.contains("editions"))
        throw runtime_error("category '" + number_ + "' has no editions");

    const json& edition_definitions = definition.at("editions");

    if (!edition_definitions.is_object())
        throw invalid_argument("category '" + number_ + "' with non-object edition definition");

    for (auto ed_def_it = edition_definitions.begin(); ed_def_it != edition_definitions.end();
         ++ed_def_it)
    {
        editions_[ed_def_it.key()] =
            std::make_shared<Edition>(ed_def_it.key(), ed_def_it.value(), definition_path);
    }

    if (editions_.count(default_edition_) != 1)
        throw invalid_argument("category '" + number_ + "' default edition '" + default_edition_ +
                               "' not defined");

    //    "ref_editions":

    if (definition.contains("ref_editions"))
    {
        const json& ref_edition_definitions = definition.at("ref_editions");

        if (!ref_edition_definitions.is_object())
            throw invalid_argument("category '" + number_ +
                                   "' with non-object REF edition definition");

        for (auto ed_def_it = ref_edition_definitions.begin();
             ed_def_it != ref_edition_definitions.end(); ++ed_def_it)
        {
            ref_editions_[ed_def_it.key()] =
                std::make_shared<REFEdition>(ed_def_it.key(), ed_def_it.value(), definition_path);
        }

        if (default_ref_edition_.size() && ref_editions_.count(default_ref_edition_) != 1)
            throw invalid_argument("category '" + number_ + "' default REF edition '" +
                                   default_ref_edition_ + "' not defined");
    }

    //    "spf_editions":

    if (definition.contains("spf_editions"))
    {
        const json& spf_edition_definitions = definition.at("spf_editions");

        if (!spf_edition_definitions.is_object())
            throw invalid_argument("category '" + number_ +
                                   "' with non-object SPF edition definition");

        for (auto ed_def_it = spf_edition_definitions.begin();
             ed_def_it != spf_edition_definitions.end(); ++ed_def_it)
        {
            spf_editions_[ed_def_it.key()] =
                std::make_shared<SPFEdition>(ed_def_it.key(), ed_def_it.value(), definition_path);
        }

        if (spf_editions_.count(default_spf_edition_) != 1)
            throw invalid_argument("category '" + number_ + "' default SPF edition '" +
                                   default_spf_edition_ + "' not defined");
    }

    //    "default_mapping" : "1.0",

    if (!definition.contains("default_mapping"))
        throw runtime_error("category '" + number_ + "' has no default mapping");

    default_mapping_ = definition.at("default_mapping");

    //    "mappings":

    if (!definition.contains("mappings"))
        throw runtime_error("category '" + number_ + "' has no mappings");

    const json& mapping_definitions = definition.at("mappings");

    if (!mapping_definitions.is_object())
        throw invalid_argument("category '" + number_ + "' with non-object mapping definition");

    for (auto map_def_it = mapping_definitions.begin(); map_def_it != mapping_definitions.end();
         ++map_def_it)
    {
        mappings_[map_def_it.key()] = std::shared_ptr<Mapping>(
            new Mapping(map_def_it.key(), map_def_it.value(), definition_path));
    }

    if (default_mapping_.size() && mappings_.count(default_mapping_) != 1)
        throw invalid_argument("category '" + number_ + "' default mapping '" + default_mapping_ +
                               "' not defined");

    current_edition_ = default_edition_;
    current_ref_edition_ = default_ref_edition_;
    current_spf_edition_ = default_spf_edition_;
    current_mapping_ = default_mapping_;
}

Category::~Category() {}

std::string Category::number() const { return number_; }

std::string Category::comment() const { return comment_; }

// edition stuff
bool Category::hasEdition(const std::string& edition_str) const
{
    return editions_.count(edition_str) == 1;
}

std::shared_ptr<Edition> Category::edition(const std::string& edition_str)
{
    traced_assert(hasEdition(edition_str));
    return editions_.at(edition_str);
}

std::string Category::editionPath(const std::string& edition_str) const
{
    traced_assert(hasEdition(edition_str));
    return editions_.at(edition_str)->definitionPath();
}

std::string Category::defaultEdition() const { return default_edition_; }

void Category::setCurrentEdition(const std::string& edition_str)
{
    traced_assert(hasEdition(edition_str));
    current_edition_ = edition_str;
}

std::shared_ptr<Edition> Category::getCurrentEdition()
{
    traced_assert(hasEdition(current_edition_));
    return editions_.at(current_edition_);
}

const std::map<std::string, std::shared_ptr<Edition>>& Category::editions() const
{
    return editions_;
}

// ref stuff
bool Category::hasREFEdition(const std::string& edition_str) const
{
    return ref_editions_.count(edition_str) == 1;
}

std::shared_ptr<REFEdition> Category::refEdition(const std::string& edition_str)
{
    traced_assert(hasREFEdition(edition_str));
    return ref_editions_.at(edition_str);
}

std::string Category::refEditionPath(const std::string& edition_str) const
{
    traced_assert(hasREFEdition(edition_str));
    return ref_editions_.at(edition_str)->definitionPath();
}

std::string Category::defaultREFEdition() const { return default_ref_edition_; }

void Category::setCurrentREFEdition(const std::string& edition_str)
{
    if (edition_str.size())  // empty is clear
        traced_assert(hasREFEdition(edition_str));

    current_ref_edition_ = edition_str;
}

bool Category::hasCurrentREFEdition() { return hasREFEdition(current_ref_edition_); }

std::shared_ptr<REFEdition> Category::getCurrentREFEdition()
{
    traced_assert(hasREFEdition(current_ref_edition_));
    return ref_editions_.at(current_ref_edition_);
}

const std::map<std::string, std::shared_ptr<REFEdition>>& Category::refEditions() const
{
    return ref_editions_;
}

// spf stuff
bool Category::hasSPFEdition(const std::string& edition_str) const
{
    return spf_editions_.count(edition_str) == 1;
}

std::shared_ptr<SPFEdition> Category::spfEdition(const std::string& edition_str)
{
    traced_assert(hasSPFEdition(edition_str));
    return spf_editions_.at(edition_str);
}

std::string Category::spfEditionPath(const std::string& edition_str) const
{
    traced_assert(hasSPFEdition(edition_str));
    return spf_editions_.at(edition_str)->definitionPath();
}

std::string Category::defaultSPFEdition() const { return default_spf_edition_; }

void Category::setCurrentSPFEdition(const std::string& edition_str)
{
    if (edition_str.size())  // empty is clear
        traced_assert(hasSPFEdition(edition_str));

    current_spf_edition_ = edition_str;
}

bool Category::hasCurrentSPFEdition() { return hasSPFEdition(current_spf_edition_); }

std::shared_ptr<SPFEdition> Category::getCurrentSPFEdition()
{
    traced_assert(hasSPFEdition(current_spf_edition_));
    return spf_editions_.at(current_spf_edition_);
}

const std::map<std::string, std::shared_ptr<SPFEdition>>& Category::spfEditions() const
{
    return spf_editions_;
}

// mapping stuff
bool Category::hasMapping(const std::string& mapping_str)
{
    return mappings_.count(mapping_str) == 1;
}

std::shared_ptr<Mapping> Category::mapping(const std::string& mapping_str)
{
    traced_assert(hasMapping(mapping_str));
    return mappings_.at(mapping_str);
}

std::string Category::defaultMapping() const { return default_mapping_; }

bool Category::hasCurrentMapping()
{
    if (current_mapping_.size() == 0)
        return false;

    return mappings_.count(current_mapping_) == 1;
}

void Category::setCurrentMapping(const std::string& mapping_str)
{
    if (mapping_str.size() == 0)  // erase current mapping
    {
        current_mapping_ = "";
        return;
    }

    traced_assert(hasMapping(mapping_str));
    current_mapping_ = mapping_str;
}

std::shared_ptr<Mapping> Category::getCurrentMapping()
{
    traced_assert(hasCurrentMapping());
    return mappings_.at(current_mapping_);
}

const std::map<std::string, std::shared_ptr<Mapping>>& Category::mappings() const
{
    return mappings_;
}

bool Category::decode() const { return decode_; }

void Category::decode(bool value) { decode_ = value; }


CategoryItemInfo Category::itemInfo () const
{
    CategoryItemInfo info;

    for (const auto& ed_it : editions_)
        ed_it.second->addInfo(ed_it.first, info);

    for (const auto& ed_it : ref_editions_)
        ed_it.second->addInfo("REF "+ed_it.first, info);

    for (const auto& ed_it : spf_editions_)
        ed_it.second->addInfo("SPF "+ed_it.first, info);

    return info;
}

}  // namespace jASTERIX
