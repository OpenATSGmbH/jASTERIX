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
#include "string_conv.h"
#include "test_jasterix.h"

using namespace std;
using namespace nlohmann;

// cat240ed1.3.bin: synthetic data, two data blocks with one record each
// block 1: Video Summary message
//   f0 00 1e | d1 08 | 07 2a | 01 | 12 "TEST VIDEO SUMMARY" | 46 50 40
// block 2: Video message
//   f0 00 2b | eb c8 | 07 2a | 02 | 00 00 00 01 | 40 00 40 5b 00 00 00 0a 00 00 00 64
//   | 00 04 | 00 08 00 00 08 | 02 01 02 03 04 05 06 07 08 | 46 50 60

void test_cat240_callback(std::unique_ptr<nlohmann::json> json_data, size_t total_num_bytes, size_t num_frames,
                          size_t num_records, size_t num_errors)
{
    loginf << "cat240 test: decoded " << num_frames << " frames, " << num_records << " records, "
           << num_errors << " errors" << logendl;

    REQUIRE(num_frames == 0);
    REQUIRE(num_records == 2);
    REQUIRE(num_errors == 0);

    REQUIRE(json_data->contains("data_blocks"));
    REQUIRE(json_data->at("data_blocks").is_array());
    REQUIRE(json_data->at("data_blocks").size() == 2);

    // block 1, Video Summary message

    loginf << "cat240 test: video summary message" << logendl;

    const json& first_data_block = json_data->at("data_blocks").at(0);

    REQUIRE(first_data_block.at("category") == 240);
    REQUIRE(first_data_block.at("length") == 30);
    REQUIRE(first_data_block.at("content").at("records").size() == 1);

    const json& summary = first_data_block.at("content").at("records").at(0);

    REQUIRE(summary.at("010").at("SAC") == 7);
    REQUIRE(summary.at("010").at("SIC") == 42);

    REQUIRE(summary.at("000").at("Message Type") == 1);

    // repetitive item: REP counts only the characters, so no additative_factor
    REQUIRE(summary.at("030").at("REP") == 18);
    REQUIRE(summary.at("030").at("Video Summary") == "TEST VIDEO SUMMARY");

    REQUIRE(approximatelyEqual(summary.at("140").at("Time of Day"), 36000.5, 10e-6));

    REQUIRE(!summary.contains("020"));
    REQUIRE(!summary.contains("048"));

    // block 2, Video message

    loginf << "cat240 test: video message" << logendl;

    const json& second_data_block = json_data->at("data_blocks").at(1);

    REQUIRE(second_data_block.at("category") == 240);
    REQUIRE(second_data_block.at("length") == 43);
    REQUIRE(second_data_block.at("content").at("records").size() == 1);

    const json& video = second_data_block.at("content").at("records").at(0);

    REQUIRE(video.at("010").at("SAC") == 7);
    REQUIRE(video.at("010").at("SIC") == 42);

    REQUIRE(video.at("000").at("Message Type") == 2);

    REQUIRE(video.at("020").at("MSG_INDEX") == 1);

    REQUIRE(approximatelyEqual(video.at("040").at("START_AZ"), 90.0, 10e-6));
    REQUIRE(approximatelyEqual(video.at("040").at("END_AZ"), 90.4998779296875, 10e-6));
    REQUIRE(video.at("040").at("START_RG") == 10);
    REQUIRE(video.at("040").at("CELL_DUR") == 100);

    REQUIRE(video.at("048").at("C") == 0);
    REQUIRE(video.at("048").at("RES") == 4);

    REQUIRE(video.at("049").at("NB_VB") == 8);
    REQUIRE(video.at("049").at("NB_CELLS") == 8);

    const json& blocks = video.at("050").at("Video Block Low Data Volume");
    REQUIRE(blocks.is_array());
    REQUIRE(blocks.size() == 2);
    REQUIRE(blocks.at(0).at("VIDEO_BLOCK") == "01020304");
    REQUIRE(blocks.at(1).at("VIDEO_BLOCK") == "05060708");

    REQUIRE(approximatelyEqual(video.at("140").at("Time of Day"), 36000.75, 10e-6));

    REQUIRE(!video.contains("030"));
    REQUIRE(!video.contains("041"));
}

TEST_CASE("jASTERIX CAT240 1.3", "[jASTERIX CAT240]")
{
    loginf << "cat240 test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, true, true, false);

    REQUIRE(jasterix.hasCategory(240));
    std::shared_ptr<jASTERIX::Category> cat240 = jasterix.category(240);
    REQUIRE(cat240->hasEdition("1.3"));
    cat240->setCurrentEdition("1.3");

    const std::string filename = "cat240ed1.3.bin";

    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));
    REQUIRE(jASTERIX::Files::fileSize(data_path + filename) == 73);

    jasterix.decodeFile(data_path + filename, test_cat240_callback);

    loginf << "cat240 test: end" << logendl;
}

static size_t decode_cat240_errors(const std::vector<char>& data)
{
    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);
    jasterix.category(240)->setCurrentEdition("1.3");

    size_t errors = 0;

    jasterix.decodeData(data.data(), data.size(),
                        [&](std::unique_ptr<nlohmann::json>, size_t, size_t, size_t,
                            size_t num_errors) { errors = num_errors; },
                        false);
    return errors;
}

TEST_CASE("jASTERIX CAT240 1.3 Video Summary truncated", "[jASTERIX CAT240]")
{
    // REP 18, but only 4 characters before the data block ends
    const std::vector<char> data = {char(0xf0), 0x00, 0x0c, char(0xd0), 0x07, 0x2a, 0x01,
                                    0x12,       'T',  'E',  'S',        'T'};

    REQUIRE(decode_cat240_errors(data) > 0);
}

TEST_CASE("jASTERIX CAT240 1.3 Video Summary non-ASCII", "[jASTERIX CAT240]")
{
    const std::vector<char> data = {char(0xf0), 0x00, 0x0c, char(0xd0), 0x07, 0x2a, 0x01,
                                    0x04,       'T',  char(0xe9), 'S',  'T'};

    REQUIRE(decode_cat240_errors(data) > 0);
}
