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

#include <cstdio>

#include "catch.hpp"
#include "jasterix.h"
#include "logger.h"
#include "test_jasterix.h"
#include "test_synthetic_files.h"

using namespace std;
using namespace nlohmann;
using namespace synthetic;

// The analysis (COMPASS import probe) runs on the flat columns: per data source and category
// the record count, per item path (flat column name) the count of present values and, for
// scalar values, min and max. Repetitive leaves are counted, record bookkeeping keys are not
// part of it.
//
// Ten copies of the CAT247 test record: SAC/SIC 0/1, Time of Day 14431.4609375, three
// entries in I247/550.

TEST_CASE("jASTERIX analysis statistics", "[jASTERIX analysis]")
{
    loginf << "analysis test: start" << logendl;

    std::vector<unsigned char> raw;
    for (unsigned int cnt = 0; cnt < 10; ++cnt)
        raw.insert(raw.end(), cat247_data_block.begin(), cat247_data_block.end());

    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);
    jasterix.category(247)->setCurrentEdition("1.2");

    std::unique_ptr<json> result = jasterix.analyzeData(
        reinterpret_cast<const char*>(raw.data()), raw.size());

    REQUIRE(result->at("num_records") == 10);
    REQUIRE(result->at("num_errors") == 0);
    REQUIRE(!result->contains("skipped_categories"));

    // one data source, one category
    REQUIRE(result->contains("0/1"));
    const json& sensor = result->at("0/1");
    REQUIRE(sensor.size() == 1);
    REQUIRE(sensor.contains("247"));

    const json& cat = sensor.at("247");
    REQUIRE(cat.at("count") == 10);

    // scalar leaves: count, min, max
    REQUIRE(cat.at("010.SAC").at("count") == 10);
    REQUIRE(cat.at("010.SAC").at("min") == 0);
    REQUIRE(cat.at("010.SAC").at("max") == 0);
    REQUIRE(cat.at("010.SIC").at("min") == 1);
    REQUIRE(cat.at("015.Service Identification").at("count") == 10);
    REQUIRE(approximatelyEqual(cat.at("140.Time-of-Day").at("min"), 14431.4609375, 1e-6));
    REQUIRE(approximatelyEqual(cat.at("140.Time-of-Day").at("max"), 14431.4609375, 1e-6));

    // repetitive item: REP as scalar, the leaves per record as arrays, counted only
    REQUIRE(cat.at("550.REP").at("min") == 3);
    REQUIRE(cat.at("550.REP").at("max") == 3);
    REQUIRE(cat.at("550.Category Version Number Report.CAT").at("count") == 10);
    REQUIRE(!cat.at("550.Category Version Number Report.CAT").contains("min"));

    // record bookkeeping is not a data item
    REQUIRE(!cat.contains("FSPEC"));
    REQUIRE(!cat.contains("index"));
    REQUIRE(!cat.contains("length"));

    // CSV output lists the same values
    std::string csv = jasterix.analyzeDataCSV(reinterpret_cast<const char*>(raw.data()), raw.size());
    REQUIRE(csv.find("0/1 CAT247:") != std::string::npos);
    REQUIRE(csv.find("0/1;010.SAC;10;0;0") != std::string::npos);
    REQUIRE(csv.find("0/1;550.REP;10;3;3") != std::string::npos);

    loginf << "analysis test: end" << logendl;
}

TEST_CASE("jASTERIX analysis skipped categories", "[jASTERIX analysis]")
{
    loginf << "analysis skipped test: start" << logendl;

    std::vector<unsigned char> raw(cat247_data_block);

    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);
    jasterix.category(247)->setCurrentEdition("1.2");
    jasterix.setDecodeCategory(247, false);

    std::unique_ptr<json> result = jasterix.analyzeData(
        reinterpret_cast<const char*>(raw.data()), raw.size());

    REQUIRE(result->at("num_records") == 0);
    REQUIRE(result->contains("skipped_categories"));
    REQUIRE(result->at("skipped_categories").at("247").at("data_blocks") == 1);
    REQUIRE(result->at("skipped_categories").at("247").at("bytes") == 20);
    REQUIRE(result->at("skipped_categories").at("247").at("reason") == "decoding disabled");

    loginf << "analysis skipped test: end" << logendl;
}
