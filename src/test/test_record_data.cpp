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

using namespace std;
using namespace nlohmann;

// Record data of the cat062 1.12 test record, taken in structured mode and compared
// against the flat mode result.
static std::string expected_record_data;

static bool is_lowercase_hex(const std::string& s)
{
    if (s.empty() || s.size() % 2 != 0)
        return false;

    for (char c : s)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            return false;

    return true;
}

static void test_record_data_structured_callback(std::unique_ptr<nlohmann::json> json_data,
                                                 size_t total_num_bytes, size_t num_frames,
                                                 size_t num_records, size_t num_errors)
{
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

    REQUIRE(json_data->contains("data_blocks"));
    const json& first_data_block = json_data->at("data_blocks").at(0);
    REQUIRE(first_data_block.at("category") == 62);

    const json& record = first_data_block.at("content").at("records").at(0);

    REQUIRE(record.contains("record_data"));
    REQUIRE(record.at("record_data").is_string());

    std::string record_data = record.at("record_data").get<std::string>();
    REQUIRE(is_lowercase_hex(record_data));

    // the hex must cover exactly the reported record length
    REQUIRE(record.contains("length"));
    REQUIRE(record_data.size() == 2 * record.at("length").get<size_t>());

    expected_record_data = record_data;

    loginf << "record_data structured test: " << record_data << logendl;
}

static void test_record_data_flat_callback(std::unique_ptr<nlohmann::json> json_data,
                                           size_t total_num_bytes, size_t num_frames,
                                           size_t num_records, size_t num_errors)
{
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

    // flat mode output is keyed by category number as string
    REQUIRE(json_data->contains("62"));
    const json& cat62 = json_data->at("62");

    REQUIRE(cat62.contains("record_data"));
    REQUIRE(cat62.at("record_data").is_array());
    REQUIRE(cat62.at("record_data").size() == 1);
    REQUIRE(cat62.at("record_data").at(0).is_string());

    std::string record_data = cat62.at("record_data").at(0).get<std::string>();
    REQUIRE(is_lowercase_hex(record_data));

    loginf << "record_data flat test: " << record_data << logendl;

    // must match the structured mode result for the same data
    REQUIRE(!expected_record_data.empty());
    REQUIRE(record_data == expected_record_data);
}

static void test_record_data_disabled_callback(std::unique_ptr<nlohmann::json> json_data,
                                               size_t total_num_bytes, size_t num_frames,
                                               size_t num_records, size_t num_errors)
{
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

    REQUIRE(json_data->contains("62"));

    // no column at all when the option is off
    REQUIRE(!json_data->at("62").contains("record_data"));
}

TEST_CASE("jASTERIX record data structured mode", "[jASTERIX record data]")
{
    loginf << "record_data structured test: start" << logendl;

    jASTERIX::add_record_data = true;

    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);

    REQUIRE(jasterix.hasCategory(62));
    std::shared_ptr<jASTERIX::Category> cat062 = jasterix.category(62);
    REQUIRE(cat062->hasEdition("1.12"));
    cat062->setCurrentEdition("1.12");

    const std::string filename = "cat062ed1.12.bin";
    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));

    jasterix.decodeFile(data_path + filename, test_record_data_structured_callback);

    jASTERIX::add_record_data = false;

    loginf << "record_data structured test: end" << logendl;
}

TEST_CASE("jASTERIX record data flat mode", "[jASTERIX record data]")
{
    loginf << "record_data flat test: start" << logendl;

    // structured test must have run first to fill expected_record_data
    REQUIRE(!expected_record_data.empty());

    jASTERIX::add_record_data = true;

    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);

    REQUIRE(jasterix.hasCategory(62));
    std::shared_ptr<jASTERIX::Category> cat062 = jasterix.category(62);
    REQUIRE(cat062->hasEdition("1.12"));
    cat062->setCurrentEdition("1.12");

    const std::string filename = "cat062ed1.12.bin";
    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));

    jasterix.decodeFile(data_path + filename, test_record_data_flat_callback, true);

    jASTERIX::add_record_data = false;

    loginf << "record_data flat test: end" << logendl;
}

TEST_CASE("jASTERIX record data disabled", "[jASTERIX record data]")
{
    loginf << "record_data disabled test: start" << logendl;

    jASTERIX::add_record_data = false;

    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);

    REQUIRE(jasterix.hasCategory(62));
    std::shared_ptr<jASTERIX::Category> cat062 = jasterix.category(62);
    REQUIRE(cat062->hasEdition("1.12"));
    cat062->setCurrentEdition("1.12");

    const std::string filename = "cat062ed1.12.bin";
    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));

    jasterix.decodeFile(data_path + filename, test_record_data_disabled_callback, true);

    loginf << "record_data disabled test: end" << logendl;
}
