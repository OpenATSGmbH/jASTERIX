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

#include <cmath>

#include "catch.hpp"
#include "files.h"
#include "jasterix.h"
#include "logger.h"
#include "string_conv.h"
#include "test_jasterix.h"

using namespace std;
using namespace nlohmann;

void test_cat002_callback(std::unique_ptr<nlohmann::json> json_data, size_t num_frames,
                          size_t num_records, size_t num_errors)
{
    loginf << "cat002 test: decoded " << num_frames << " frames, " << num_records << " records, "
           << num_errors << " errors" << logendl;

    REQUIRE(num_frames == 0);
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

    //    {
    //        "data_blocks": [
    //            {
    //                "category": 2,
    //                "content": {
    //                    "index": 3,
    //                    "length": 9,
    //                    "records": [
    //                        {
    //                            "000": {
    //                                "Message Type": 1
    //                            },
    //                            "010": {
    //                                "SAC": 0,
    //                                "SIC": 1
    //                            },
    //                            "030": {
    //                                "Time of Day": 33501.4140625
    //                            },
    //                            "050": {
    //                                "Station Configuration Status": [
    //                                    {
    //                                        "Value": 73,
    //                                        "extend": 1
    //                                    },
    //                                    {
    //                                        "Value": 1,
    //                                        "extend": 0
    //                                    }
    //                                ]
    //                            },
    //                            "FSPEC": [
    //                                true,
    //                                true,
    //                                false,
    //                                true,
    //                                false,
    //                                true,
    //                                false,
    //                                false
    //                            ]
    //                        }
    //                    ]
    //                },
    //                "length": 12
    //            }
    //        ]
    //    }

    loginf << "cat002 test: data block" << logendl;

    REQUIRE(json_data->contains("data_blocks"));
    REQUIRE(json_data->at("data_blocks").is_array());
    REQUIRE(json_data->at("data_blocks").size() == 1);

    const json& first_data_block = json_data->at("data_blocks").at(0);

    REQUIRE(first_data_block.contains("category"));
    REQUIRE(first_data_block.at("category") == 2);
    REQUIRE(first_data_block.contains("length"));
    REQUIRE(first_data_block.at("length") == 12);

    //    ; Netto frame 1 (length=12) at offset 0x00000000 (0):
    //      0x 02 00 0c d4  00 01 01 41  6e b5 93 02
    //    ; Netto frame 1:
    //    ; ASTERIX data block at pos 0: cat=2; len=12
    //      0x 02 00 0c d4  00 01 01 41  6e b5 93 02

    loginf << "cat002 test: num records" << logendl;
    REQUIRE(first_data_block.contains("content"));
    REQUIRE(first_data_block.at("content").contains("records"));
    REQUIRE(first_data_block.at("content").at("records").is_array());
    REQUIRE(first_data_block.at("content").at("records").size() == 1);

    const json& record = first_data_block.at("content").at("records").at(0);

    //    ; FSPEC: 0x d4

    loginf << "cat002 test: fspec" << logendl;
    REQUIRE(record.at("FSPEC").size() == 8);

    REQUIRE(record.at("FSPEC") == std::vector<bool>({1, 1, 0, 1, 0, 1, 0, 0}));

    //    ; Data Record:
    //    ;  I002/010: =0x 00 01
    //    ;  Data Source Identifier: 0x0001 (SAC=0; SIC=1)

    loginf << "cat002 test: 010" << logendl;
    REQUIRE(record.at("010").at("SAC") == 0);
    REQUIRE(record.at("010").at("SIC") == 1);

    //    ;  I002/000: =0x 01
    //    ;  Message Type: mtp=1 (North marker message)
    loginf << "cat002 test: 000" << logendl;
    REQUIRE(record.at("000").at("Message Type") == 1);

    //    ;  I002/030: =0x 41 6e b5
    //    ;  Time of Day: 0x416eb5 (4288181; 33501.414062 secs; 09:18:21.414 UTC)
    loginf << "cat002 test: 030" << logendl;
    REQUIRE(approximatelyEqual(record.at("030").at("Time of Day"), 33501.4140625, 10e-6));

    //    ;  I002/050: =0x 93 02
    //    ;  Station Configuration Status: 0x 92 02
    loginf << "cat002 test: 050" << logendl;
    REQUIRE(record.at("050").at("Station Configuration Status").size() == 2);
    REQUIRE(record.at("050").at("Station Configuration Status")[0].at("Value") == 73);
    REQUIRE(record.at("050").at("Station Configuration Status")[1].at("Value") == 1);

    //     [--:--:--.---] M 09:18:21.414 -- 0x0001 A1 S:NMK ----- A-    0.000
}

TEST_CASE("jASTERIX CAT002 1.0", "[jASTERIX CAT002]")
{
    loginf << "cat002 test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, true, true, false);

    //      02000cd4000101416eb59302
    // echo -n 02000cd4000101416eb59302 | xxd -r -p > cat002ed1.0.bin

    REQUIRE(jasterix.hasCategory(2));
    std::shared_ptr<jASTERIX::Category> cat002 = jasterix.category(2);
    REQUIRE(cat002->hasEdition("1.0"));
    cat002->setCurrentEdition("1.0");
    cat002->setCurrentMapping("");

    const std::string filename = "cat002ed1.0.bin";

    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));
    REQUIRE(jASTERIX::Files::fileSize(data_path + filename) == 12);

    jasterix.decodeFile(data_path + filename, test_cat002_callback);

    loginf << "cat002 test: end" << logendl;
}
