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

void test_cat004_callback(std::unique_ptr<nlohmann::json> json_data, size_t total_num_bytes, size_t num_frames,
                          size_t num_records, size_t num_errors)
{
    loginf << "cat004 test: decoded " << num_frames << " frames, " << num_records << " records, "
           << num_errors << " errors" << logendl;

    REQUIRE(num_frames == 0);
    REQUIRE(num_records == 3);
    REQUIRE(num_errors == 0);

    // {
    //     "data_blocks": [
    //         {
    //             "category": 4,
    //             "content": {
    //                 "index": 3,
    //                 "length": 8,
    //                 "records": [
    //                     {
    //                         "000": {
    //                             "Message Type": 1
    //                         },
    //                         "010": {
    //                             "SAC": 0,
    //                             "SIC": 1
    //                         },
    //                         "020": {
    //                             "TIME OF MESSAGE": 2829.0
    //                         },
    //                         "060": {
    //                             "APW": 1,
    //                             "CLAM": 1,
    //                             "FX": 0,
    //                             "MRVA": 0,
    //                             "MSAW": 0,
    //                             "RAMHD": 0,
    //                             "RAMLD": 0,
    //                             "STCA": 1
    //                         },
    //                         "FSPEC": [
    //                             true,
    //                             true,
    //                             false,
    //                             true,
    //                             false,
    //                             false,
    //                             true,
    //                             false
    //                         ],
    //                         "index": 3,
    //                         "length": 8,
    //                         "record_data": "d20001010586800e"
    //                     }
    //                 ]
    //             },
    //             "length": 11
    //         },
    //         {
    //             "category": 4,
    //             "content": {
    //                 "index": 14,
    //                 "length": 12,
    //                 "records": [
    //                     {
    //                         "000": {
    //                             "Message Type": 6
    //                         },
    //                         "010": {
    //                             "SAC": 0,
    //                             "SIC": 1
    //                         },
    //                         "020": {
    //                             "TIME OF MESSAGE": 70666.0
    //                         },
    //                         "030": {
    //                             "Track Number 1": 3027
    //                         },
    //                         "040": {
    //                             "Alert Identifier": 15496
    //                         },
    //                         "FSPEC": [
    //                             true,
    //                             true,
    //                             false,
    //                             true,
    //                             true,
    //                             false,
    //                             false,
    //                             true,
    //                             true,
    //                             false,
    //                             false,
    //                             false,
    //                             false,
    //                             false,
    //                             false,
    //                             false
    //                         ],
    //                         "index": 14,
    //                         "length": 12,
    //                         "record_data": "d9800001068a05003c880bd3"
    //                     }
    //                 ]
    //             },
    //             "length": 15
    //         },
    //         {
    //             "category": 4,
    //             "content": {
    //                 "index": 29,
    //                 "length": 17,
    //                 "records": [
    //                     {
    //                         "000": {
    //                             "Message Type": 7
    //                         },
    //                         "010": {
    //                             "SAC": 0,
    //                             "SIC": 1
    //                         },
    //                         "020": {
    //                             "TIME OF MESSAGE": 70658.0
    //                         },
    //                         "030": {
    //                             "Track Number 1": 3130
    //                         },
    //                         "035": {
    //                             "Track Number 2": 2417
    //                         },
    //                         "040": {
    //                             "Alert Identifier": 15497
    //                         },
    //                         "045": {
    //                             "Alert status": 3
    //                         },
    //                         "120": {
    //                             "available": [
    //                                 false,
    //                                 false,
    //                                 false,
    //                                 false,
    //                                 false,
    //                                 false,
    //                                 false,
    //                                 false
    //                             ]
    //                         },
    //                         "FSPEC": [
    //                             true,
    //                             true,
    //                             false,
    //                             true,
    //                             true,
    //                             true,
    //                             false,
    //                             true,
    //                             true,
    //                             false,
    //                             true,
    //                             false,
    //                             false,
    //                             false,
    //                             false,
    //                             true,
    //                             false,
    //                             true,
    //                             false,
    //                             false,
    //                             false,
    //                             false,
    //                             false,
    //                             false
    //                         ],
    //                         "index": 29,
    //                         "length": 17,
    //                         "record_data": "dda1400001078a01003c89060c3a000971"
    //                     }
    //                 ]
    //             },
    //             "length": 20
    //         }
    //     ],
    //     "rec_num": 0
    // }

    loginf << "cat004 test: data block" << logendl;

    REQUIRE(json_data->contains("data_blocks"));
    REQUIRE(json_data->at("data_blocks").is_array());
    REQUIRE(json_data->at("data_blocks").size() == 3);

    // ----------------------------------
    // --- first frame = Alive
    // ----------------------------------

    const json& first_data_block = json_data->at("data_blocks").at(0);

    REQUIRE(first_data_block.contains("category"));
    REQUIRE(first_data_block.at("category") == 4);
    REQUIRE(first_data_block.contains("length"));
    REQUIRE(first_data_block.at("length") == 11);

    loginf << "cat004 test #1: num records" << logendl;
    REQUIRE(first_data_block.contains("content"));
    REQUIRE(first_data_block.at("content").contains("records"));
    REQUIRE(first_data_block.at("content").at("records").is_array());
    REQUIRE(first_data_block.at("content").at("records").size() == 1);

    const json& record1 = first_data_block.at("content").at("records").at(0);

    //    ; FSPEC: 0x d2
    //    11010010

    loginf << "cat004 test #1: fspec" << logendl;


    // ;  I004/010: =0x 00 01
    // ;  Data Source Identifier: 0x0001 (SAC=0; SIC=1)

    loginf << "cat004 test #1: 010" << logendl;
    REQUIRE(record1.at("010").at("SAC") == 0);
    REQUIRE(record1.at("010").at("SIC") == 1);

    // ;  I004/000: =0x 01
    // ;  Message Type: 1 (Alive Message)

    loginf << "cat004 test #1: 000" << logendl;
    REQUIRE(record1.at("000").at("Message Type") == 1);

    // ;  I004/020: =0x 05 86 80
    // ;  Time of Day: 0x058680 (362112; 2829.000000 secs; 00:47:09.000 UTC)

    loginf << "cat004 test #1: 020" << logendl;
    REQUIRE(record1.at("020").at("TIME OF MESSAGE") == 2829.0);

    // ;  I004/060: =0x 0e
    // ;  Safety Net Function Status:
    // ;   ix=1: 0x0e
    // ;   RAMLD=0; RAMHD=0; MSAW=0; APW=1; CLAM=1; STCA=1

    loginf << "cat004 test #1: 060" << logendl;
    REQUIRE(record1.at("060").at("APW") == 1);
    REQUIRE(record1.at("060").at("CLAM") == 1);
    REQUIRE(record1.at("060").at("MRVA") == 0);
    REQUIRE(record1.at("060").at("MSAW") == 0);
    REQUIRE(record1.at("060").at("RAMHD") == 0);
    REQUIRE(record1.at("060").at("RAMLD") == 0);
    REQUIRE(record1.at("060").at("STCA") == 1);

    // ----------------------------------
    // --- second frame = CLAM
    // ----------------------------------

    const json& second_data_block = json_data->at("data_blocks").at(1);

    REQUIRE(second_data_block.contains("category"));
    REQUIRE(second_data_block.at("category") == 4);
    REQUIRE(second_data_block.contains("length"));
    REQUIRE(second_data_block.at("length") == 15);

    loginf << "cat004 test #2: num records" << logendl;
    REQUIRE(second_data_block.contains("content"));
    REQUIRE(second_data_block.at("content").contains("records"));
    REQUIRE(second_data_block.at("content").at("records").is_array());
    REQUIRE(second_data_block.at("content").at("records").size() == 1);

    const json& record2 = second_data_block.at("content").at("records").at(0);

    //    ; FSPEC: 0x d9 80
    //    11011001 10000000

    loginf << "cat004 test #2: fspec" << logendl;


    // ;  I004/010: =0x 00 01
    // ;  Data Source Identifier: 0x0001 (SAC=0; SIC=1)

    loginf << "cat004 test #2: 010" << logendl;
    REQUIRE(record2.at("010").at("SAC") == 0);
    REQUIRE(record2.at("010").at("SIC") == 1);

    // ;  I004/000: =0x 06
    // ;  Message Type: 6 (Clearance Level Adherence Monitor)

    loginf << "cat004 test #2: 000" << logendl;
    REQUIRE(record2.at("000").at("Message Type") == 6);

    // ;  I004/020: =0x 8a 05 00
    // ;  Time of Day: 0x8a0500 (9045248; 70666.000000 secs; 19:37:46.000 UTC)

    loginf << "cat004 test #2: 020" << logendl;
    REQUIRE(record2.at("020").at("TIME OF MESSAGE") == 70666.0);

    // ;  I004/040: =0x 3c 88
    // ;  Alert Identifier: 15496

    loginf << "cat004 test #2: 040" << logendl;
    REQUIRE(record2.at("040").at("Alert Identifier") == 15496);

    // ;  I004/030: =0x 0b d3
    // ;  Track Number 1: 3027

    loginf << "cat004 test #2: 030" << logendl;
    REQUIRE(record2.at("030").at("Track Number 1") == 3027);

    // ----------------------------------
    // --- third frame = STCA
    // ----------------------------------

    const json& third_data_block = json_data->at("data_blocks").at(2);

    REQUIRE(third_data_block.contains("category"));
    REQUIRE(third_data_block.at("category") == 4);
    REQUIRE(third_data_block.contains("length"));
    REQUIRE(third_data_block.at("length") == 20);

    loginf << "cat004 test #3: num records" << logendl;
    REQUIRE(third_data_block.contains("content"));
    REQUIRE(third_data_block.at("content").contains("records"));
    REQUIRE(third_data_block.at("content").at("records").is_array());
    REQUIRE(third_data_block.at("content").at("records").size() == 1);

    const json& record3 = third_data_block.at("content").at("records").at(0);

    //    ; FSPEC: 0x dd a1 40
    //    11011101 10100001 01000000

    loginf << "cat004 test #3: fspec" << logendl;


    // ;  I004/010: =0x 00 01
    // ;  Data Source Identifier: 0x0001 (SAC=0; SIC=1)

    loginf << "cat004 test #3: 010" << logendl;
    REQUIRE(record3.at("010").at("SAC") == 0);
    REQUIRE(record3.at("010").at("SIC") == 1);

    // ;  I004/000: =0x 07
    // ;  Message Type: 7 (Short Term Conflict Alert)

    loginf << "cat004 test #3: 000" << logendl;
    REQUIRE(record3.at("000").at("Message Type") == 7);

    // ;  I004/020: =0x 8a 01 00
    // ;  Time of Day: 0x8a0100 (9044224; 70658.000000 secs; 19:37:38.000 UTC)

    loginf << "cat004 test #3: 020" << logendl;
    REQUIRE(record3.at("020").at("TIME OF MESSAGE") == 70658.0);

    // ;  I004/040: =0x 3c 89
    // ;  Alert Identifier: 15497

    loginf << "cat004 test #3: 040" << logendl;
    REQUIRE(record3.at("040").at("Alert Identifier") == 15497);

    // ;  I004/045: =0x 06
    // ;  Alert Status: 3

    loginf << "cat004 test #3: 045" << logendl;
    REQUIRE(record3.at("045").at("Alert status") == 3);

    // ;  I004/030: =0x 0c 3a
    // ;  Track Number 1: 3130

    loginf << "cat004 test #3: 030" << logendl;
    REQUIRE(record3.at("030").at("Track Number 1") == 3130);

    // ;  I004/035: =0x 09 71
    // ;  Track Number 2: 2417

    loginf << "cat004 test #3: 035" << logendl;
    REQUIRE(record3.at("035").at("Track Number 2") == 2417);
}

TEST_CASE("jASTERIX CAT004 1.4", "[jASTERIX CAT004]")
{
    loginf << "cat004 test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, true, true, false);

    // 04000bd20001010586800e04000fd9800001068a05003c880bd3040014dda1400001078a01003c89060c3a000971
    // echo -n
    // 04000bd20001010586800e04000fd9800001068a05003c880bd3040014dda1400001078a01003c89060c3a000971c
    // | xxd -r -p > cat004ed1.4.bin

    REQUIRE(jasterix.hasCategory(4));
    std::shared_ptr<jASTERIX::Category> cat004 = jasterix.category(4);
    REQUIRE(cat004->hasEdition("1.4"));
    cat004->setCurrentEdition("1.4");
    cat004->setCurrentMapping("");

    const std::string filename = "cat004ed1.4.bin";

    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));
    REQUIRE(jASTERIX::Files::fileSize(data_path + filename) == 46);

    jasterix.decodeFile(data_path + filename, test_cat004_callback);

    loginf << "cat004 ed 1.4 test: end" << logendl;
}
