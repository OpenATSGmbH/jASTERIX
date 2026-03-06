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

void test_cat252_callback(std::unique_ptr<nlohmann::json> json_data, size_t total_num_bytes, size_t num_frames,
                          size_t num_records, size_t num_errors)
{
    loginf << "cat252 test: decoded " << num_frames << " frames, " << num_records << " records, "
           << num_errors << " errors" << logendl;

    REQUIRE(num_frames == 0);
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

    // {
    //     "data_blocks": [
    //         {
    //             "category": 252,
    //             "content": {
    //                 "index": 3,
    //                 "length": 13,
    //                 "records": [
    //                     {
    //                         "010": {
    //                             "SAC": 0,
    //                             "SIC": 7
    //                         },
    //                         "015": {
    //                             "USER NUMBER": 36
    //                         },
    //                         "020": {
    //                             "TIME OF MESSAGE": 54780.03125
    //                         },
    //                         "035": {
    //                             "FAMILY": 3,
    //                             "NATURE": 2
    //                         },
    //                         "110": {
    //                             "BS": 1,
    //                             "C1": 0,
    //                             "FX": 0
    //                         },
    //                         "330": {
    //                             "REP": 1,
    //                             "SERVICE RELATED REPORT": [
    //                                 {
    //                                     "CODE": 11,
    //                                     "NATURE": 8
    //                                 }
    //                             ]
    //                         },
    //                         "FSPEC": [
    //                             true,
    //                             true,
    //                             true,
    //                             true,
    //                             true,
    //                             true,
    //                             false,
    //                             false
    //                         ],
    //                         "index": 3,
    //                         "length": 13,
    //                         "record_data": "fc000700246afe04320401400b"
    //                     }
    //                 ]
    //             },
    //             "length": 16
    //         }
    //     ],
    //     "rec_num": 0
    // }

    loginf << "cat252 test: data block" << logendl;

    REQUIRE(json_data->contains("data_blocks"));
    REQUIRE(json_data->at("data_blocks").is_array());
    REQUIRE(json_data->at("data_blocks").size() == 1);

    const json& first_data_block = json_data->at("data_blocks").at(0);

    REQUIRE(first_data_block.contains("category"));
    REQUIRE(first_data_block.at("category") == 252);
    REQUIRE(first_data_block.contains("length"));
    REQUIRE(first_data_block.at("length") == 16);

    loginf << "cat252 test: num records" << logendl;
    REQUIRE(first_data_block.contains("content"));
    REQUIRE(first_data_block.at("content").contains("records"));
    REQUIRE(first_data_block.at("content").at("records").is_array());
    REQUIRE(first_data_block.at("content").at("records").size() == 1);

    const json& record = first_data_block.at("content").at("records").at(0);

    //    ; FSPEC: 0x fc
    //    11111100

    loginf << "cat252 test: fspec" << logendl;


    // ;  I252/010: =0x 00 07
    // ;  Server Identification Tag: 0x0007 (SAC=0; SIC=7)

    loginf << "cat252 test: 010" << logendl;
    REQUIRE(record.at("010").at("SAC") == 0);
    REQUIRE(record.at("010").at("SIC") == 7);

    // ;  I252/015: =0x 00 24
    // ;  User Number: 36

    loginf << "cat252 test: 015" << logendl;
    REQUIRE(record.at("015").at("USER NUMBER") == 36);

    // ;  I252/020: =0x 6a fe 04
    // ;  Time of Message: 0x6afe04 (7011844; 15:13:00.031 UTC)

    loginf << "cat252 test: 020" << logendl;
    REQUIRE(approximatelyEqual(record.at("020").at("TIME OF MESSAGE"), 54780.03125, 10e-5));

    // ;  I252/035: =0x 32
    // ;  Type of Message: family=3; nature=2
    // ;   Server messages: track service related report

    loginf << "cat252 test: 035" << logendl;
    REQUIRE(record.at("035").at("FAMILY") == 3);
    REQUIRE(record.at("035").at("NATURE") == 2);

    // ;  I252/110: =0x 04
    // ;  Service Identification: BS

    loginf << "cat252 test: 110" << logendl;
    REQUIRE(record.at("110").at("BS") == 1);
    REQUIRE(record.at("110").at("C1") == 0);

    // ;  I252/330: =0x 01 40 0b
    // ;  Service Related Report: nature=8; code=11
    // ;   Service synchronisation report: batch/sector 11

    loginf << "cat252 test: 330" << logendl;
    REQUIRE(record.at("330").at("SERVICE RELATED REPORT").size() == 1);
    REQUIRE(record.at("330").at("SERVICE RELATED REPORT")[0].at("CODE") == 11);
    REQUIRE(record.at("330").at("SERVICE RELATED REPORT")[0].at("NATURE") == 8);
}

TEST_CASE("jASTERIX cat252 7.0", "[jASTERIX cat252]")
{
    loginf << "cat252 test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, true, true, false);

    // fc0010fc000700246afe04320401400b
    // echo -n
    // fc0010fc000700246afe04320401400b
    // | xxd -r -p > cat252ed7.0.bin

    REQUIRE(jasterix.hasCategory(252));
    std::shared_ptr<jASTERIX::Category> cat252 = jasterix.category(252);
    REQUIRE(cat252->hasEdition("7.0"));
    cat252->setCurrentEdition("7.0");
    cat252->setCurrentMapping("");

    const std::string filename = "cat252ed7.0.bin";

    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));
    REQUIRE(jASTERIX::Files::fileSize(data_path + filename) == 16);

    jasterix.decodeFile(data_path + filename, test_cat252_callback);

    loginf << "cat252 ed 7.0 test: end" << logendl;
}
