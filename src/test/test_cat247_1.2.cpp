/*
 * This file is part of COMPASS.
 *
 * COMPASS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * COMPASS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with COMPASS.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "catch.hpp"
#include "files.h"
#include "jasterix.h"
#include "logger.h"
#include "string_conv.h"
#include "test_jasterix.h"

using namespace std;
using namespace nlohmann;

void test_cat247_callback(std::unique_ptr<nlohmann::json> json_data, size_t total_num_bytes, size_t num_frames,
                          size_t num_records, size_t num_errors)
{
    loginf << "cat247 test: decoded " << num_frames << " frames, " << num_records << " records, "
           << num_errors << " errors" << logendl;

    REQUIRE(num_frames == 0);
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

    // {
    //     "frames": [
    //         {
    //             "board_number": 15,
    //             "cnt": 0,
    //             "content": {
    //                 "data_blocks": [
    //                     {
    //                         "category": 247,
    //                         "content": {
    //                             "index": 11,
    //                             "length": 17,
    //                             "records": [
    //                                 {
    //                                     "010": {
    //                                         "SAC": 0,
    //                                         "SIC": 1
    //                                     },
    //                                     "015": {
    //                                         "Service Identification": 1
    //                                     },
    //                                     "140": {
    //                                         "Time-of-Day": 14431.4609375
    //                                     },
    //                                     "550": {
    //                                         "Category Version Number Report": [
    //                                             {
    //                                                 "CAT": 21,
    //                                                 "Main version number": 2,
    //                                                 "Sub version number": 1
    //                                             },
    //                                             {
    //                                                 "CAT": 23,
    //                                                 "Main version number": 1,
    //                                                 "Sub version number": 2
    //                                             },
    //                                             {
    //                                                 "CAT": 247,
    //                                                 "Main version number": 1,
    //                                                 "Sub version number": 2
    //                                             }
    //                                         ],
    //                                         "REP": 3
    //                                     },
    //                                     "FSPEC": [
    //                                         true,
    //                                         true,
    //                                         true,
    //                                         true,
    //                                         false,
    //                                         false,
    //                                         false,
    //                                         false
    //                                     ],
    //                                     "index": 11,
    //                                     "length": 17,
    //                                     "record_data": "f00001011c2fbb03150201170102f70102"
    //                                 }
    //                             ]
    //                         },
    //                         "length": 20
    //                     }
    //                 ],
    //                 "index": 8,
    //                 "length": 20
    //             },
    //             "length": 32,
    //             "padding": 2779096485,
    //             "recording_day": 0,
    //             "time_ms": 14432.0
    //         }
    //     ],
    //     "rec_num": 0
    // }

    loginf << "cat247 test: data block" << logendl;

    REQUIRE(json_data->contains("data_blocks"));
    REQUIRE(json_data->at("data_blocks").is_array());
    REQUIRE(json_data->at("data_blocks").size() == 1);

    const json& first_data_block = json_data->at("data_blocks").at(0);

    REQUIRE(first_data_block.contains("category"));
    REQUIRE(first_data_block.at("category") == 247);
    REQUIRE(first_data_block.contains("length"));
    REQUIRE(first_data_block.at("length") == 20);

    loginf << "cat247 test: num records" << logendl;
    REQUIRE(first_data_block.contains("content"));
    REQUIRE(first_data_block.at("content").contains("records"));
    REQUIRE(first_data_block.at("content").at("records").is_array());
    REQUIRE(first_data_block.at("content").at("records").size() == 1);

    const json& record = first_data_block.at("content").at("records").at(0);

    //    ; FSPEC: 0x f0
    //    11110000

    loginf << "cat247 test: fspec" << logendl;


    // ;  I247/010: =0x 00 01
    // ;  Data Source Identifier: 0x0001 (SAC=0; SIC=1)

    loginf << "cat247 test: 010" << logendl;
    REQUIRE(record.at("010").at("SAC") == 0);
    REQUIRE(record.at("010").at("SIC") == 1);

    // ;  I247/015: =0x 01
    // ;  Service Identification: 1

    loginf << "cat247 test: 015" << logendl;
    REQUIRE(record.at("015").at("Service Identification") == 1);

    // ;  I247/140: =0x 1c 2f bb
    // ;  Time of Day: 0x1c2fbb (1847227; 14431.460938 secs; 04:00:31.461 UTC)

    loginf << "cat247 test: 140" << logendl;
    REQUIRE(approximatelyEqual(record.at("140").at("Time-of-Day"), 14431.4609375, 10e-7));

    // ;  I247/550: =0x 03 15 02 01  17 01 02 f7  01 02
    // ;  Category Version Number Report:
    // ;   Category 21: Version 2.1
    // ;   Category 23: Version 1.2
    // ;   Category 247: Version 1.2

    loginf << "cat247 test: 550" << logendl;
    REQUIRE(record.at("550").at("Category Version Number Report").size() == 3);
    REQUIRE(record.at("550").at("Category Version Number Report")[0].at("CAT") == 21);
    REQUIRE(record.at("550").at("Category Version Number Report")[0].at("Main version number") == 2);
    REQUIRE(record.at("550").at("Category Version Number Report")[0].at("Sub version number") == 1);
    REQUIRE(record.at("550").at("Category Version Number Report")[1].at("CAT") == 23);
    REQUIRE(record.at("550").at("Category Version Number Report")[1].at("Main version number") == 1);
    REQUIRE(record.at("550").at("Category Version Number Report")[1].at("Sub version number") == 2);
    REQUIRE(record.at("550").at("Category Version Number Report")[2].at("CAT") == 247);
    REQUIRE(record.at("550").at("Category Version Number Report")[2].at("Main version number") == 1);
    REQUIRE(record.at("550").at("Category Version Number Report")[2].at("Sub version number") == 2);
}

TEST_CASE("jASTERIX CAT247 1.2", "[jASTERIX CAT247]")
{
    loginf << "cat247 test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, true, true, false);

    // f70014f00001011c2fbb03150201170102f70102
    // echo -n
    // f70014f00001011c2fbb03150201170102f70102
    // | xxd -r -p > cat247ed1.2.bin

    REQUIRE(jasterix.hasCategory(247));
    std::shared_ptr<jASTERIX::Category> cat247 = jasterix.category(247);
    REQUIRE(cat247->hasEdition("1.2"));
    cat247->setCurrentEdition("1.2");
    cat247->setCurrentMapping("");

    const std::string filename = "cat247ed1.2.bin";

    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));
    REQUIRE(jASTERIX::Files::fileSize(data_path + filename) == 20);

    jasterix.decodeFile(data_path + filename, test_cat247_callback);

    loginf << "cat247 ed 1.2 test: end" << logendl;
}
