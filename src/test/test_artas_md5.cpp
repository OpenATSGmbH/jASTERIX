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

#include "catch.hpp"
#include "files.h"
#include "jasterix.h"
#include "logger.h"
#include "test_jasterix.h"

#if USE_OPENSSL

using namespace std;
using namespace nlohmann;

// Expected hash value for the cat062 1.12 test record — computed once in structured mode
// and verified to be stable across runs.
static std::string expected_hash;

static void test_artas_md5_structured_callback(std::unique_ptr<nlohmann::json> json_data,
                                                size_t total_num_bytes, size_t num_frames,
                                                size_t num_records, size_t num_errors)
{
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

    REQUIRE(json_data->contains("data_blocks"));
    const json& first_data_block = json_data->at("data_blocks").at(0);
    REQUIRE(first_data_block.at("category") == 62);

    const json& record = first_data_block.at("content").at("records").at(0);

    // artas_md5 must be present and be a non-empty string
    REQUIRE(record.contains("artas_md5"));
    REQUIRE(record.at("artas_md5").is_string());

    std::string hash = record.at("artas_md5").get<std::string>();
    REQUIRE(!hash.empty());

    // Store for cross-mode comparison
    expected_hash = hash;

    loginf << "artas_md5 structured test: hash = " << hash << logendl;
}

static void test_artas_md5_flat_callback(std::unique_ptr<nlohmann::json> json_data,
                                          size_t total_num_bytes, size_t num_frames,
                                          size_t num_records, size_t num_errors)
{
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

    // Flat mode output is keyed by category number as string
    REQUIRE(json_data->contains("62"));
    const json& cat62 = json_data->at("62");

    // artas_md5 column must exist and have one entry
    REQUIRE(cat62.contains("artas_md5"));
    REQUIRE(cat62.at("artas_md5").is_array());
    REQUIRE(cat62.at("artas_md5").size() == 1);

    // The hash must be a non-empty string
    REQUIRE(cat62.at("artas_md5").at(0).is_string());

    std::string hash = cat62.at("artas_md5").at(0).get<std::string>();
    REQUIRE(!hash.empty());

    loginf << "artas_md5 flat test: hash = " << hash << logendl;

    // Must match the structured mode hash for the same data
    REQUIRE(!expected_hash.empty());
    REQUIRE(hash == expected_hash);
}

TEST_CASE("jASTERIX ARTAS MD5 structured mode", "[jASTERIX ARTAS MD5]")
{
    loginf << "artas_md5 structured test: start" << logendl;

    jASTERIX::add_artas_md5_hash = true;

    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);

    REQUIRE(jasterix.hasCategory(62));
    std::shared_ptr<jASTERIX::Category> cat062 = jasterix.category(62);
    REQUIRE(cat062->hasEdition("1.12"));
    cat062->setCurrentEdition("1.12");
    cat062->setCurrentMapping("");

    const std::string filename = "cat062ed1.12.bin";
    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));

    jasterix.decodeFile(data_path + filename, test_artas_md5_structured_callback);

    jASTERIX::add_artas_md5_hash = false;

    loginf << "artas_md5 structured test: end" << logendl;
}

TEST_CASE("jASTERIX ARTAS MD5 flat mode", "[jASTERIX ARTAS MD5]")
{
    loginf << "artas_md5 flat test: start" << logendl;

    // Structured test must have run first to populate expected_hash
    REQUIRE(!expected_hash.empty());

    jASTERIX::add_artas_md5_hash = true;

    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);

    REQUIRE(jasterix.hasCategory(62));
    std::shared_ptr<jASTERIX::Category> cat062 = jasterix.category(62);
    REQUIRE(cat062->hasEdition("1.12"));
    cat062->setCurrentEdition("1.12");
    cat062->setCurrentMapping("");

    const std::string filename = "cat062ed1.12.bin";
    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));

    jasterix.decodeFile(data_path + filename, test_artas_md5_flat_callback, true);

    jASTERIX::add_artas_md5_hash = false;

    loginf << "artas_md5 flat test: end" << logendl;
}

#endif // USE_OPENSSL
