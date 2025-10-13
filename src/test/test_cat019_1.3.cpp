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

void test_cat019_callback(std::unique_ptr<nlohmann::json> json_data, size_t num_frames,
                          size_t num_records, size_t num_errors)
{
    loginf << "cat019 test: decoded " << num_frames << " frames, " << num_records << " records, "
           << num_errors << " errors" << logendl;

    REQUIRE(num_frames == 0);
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

    //    {
    //        "data_blocks": [
    //            {
    //                "category": 19,
    //                "content": {
    //                    "index": 3,
    //                    "length": 54,
    //                    "records": [
    //                        {
    //                            "000": {
    //                                "Message Type": 2
    //                            },
    //                            "010": {
    //                                "SAC": 0,
    //                                "SIC": 3
    //                            },
    //                            "140": {
    //                                "Time of Day": 33503.0390625
    //                            },
    //                            "550": {
    //                                "NOGO": 0,
    //                                "OVL": 0,
    //                                "TSV": 0,
    //                                "TTF": 0
    //                            },
    //                            "551": {
    //                                "TP 1a": 1,
    //                                "TP 1b": 1,
    //                                "TP 2a": 0,
    //                                "TP 2b": 1,
    //                                "TP 3a": 0,
    //                                "TP 3b": 0,
    //                                "TP 4a": 0,
    //                                "TP 4b": 0
    //                            },
    //                            "552": {
    //                                "REP": 16,
    //                                "Remote Sensor Detailed Status": [
    //                                    {
    //                                        "RS Identification": 1,
    //                                        "RS Operational": 1,
    //                                        "RS Status": 1,
    //                                        "Receiver 1090 MHz": 1,
    //                                        "Transmitter 1030 MHz": 1,
    //                                        "Transmitter 1090 MHz": 0
    //                                    },
    //                                    {
    //                                        "RS Identification": 2,
    //                                        "RS Operational": 1,
    //                                        "RS Status": 1,
    //                                        "Receiver 1090 MHz": 1,
    //                                        "Transmitter 1030 MHz": 1,
    //                                        "Transmitter 1090 MHz": 0
    //                                    },
    //                                    {
    //                                        "RS Identification": 3,
    //                                        "RS Operational": 1,
    //                                        "RS Status": 1,
    //                                        "Receiver 1090 MHz": 1,
    //                                        "Transmitter 1030 MHz": 1,
    //                                        "Transmitter 1090 MHz": 0
    //                                    },
    //                                    {
    //                                        "RS Identification": 4,
    //                                        "RS Operational": 1,
    //                                        "RS Status": 1,
    //                                        "Receiver 1090 MHz": 1,
    //                                        "Transmitter 1030 MHz": 1,
    //                                        "Transmitter 1090 MHz": 0
    //                                    },
    //                                    {
    //                                        "RS Identification": 5,
    //                                        "RS Operational": 1,
    //                                        "RS Status": 1,
    //                                        "Receiver 1090 MHz": 1,
    //                                        "Transmitter 1030 MHz": 1,
    //                                        "Transmitter 1090 MHz": 0
    //                                    },
    //                                    {
    //                                        "RS Identification": 6,
    //                                        "RS Operational": 1,
    //                                        "RS Status": 1,
    //                                        "Receiver 1090 MHz": 1,
    //                                        "Transmitter 1030 MHz": 1,
    //                                        "Transmitter 1090 MHz": 0
    //                                    },
    //                                    {
    //                                        "RS Identification": 7,
    //                                        "RS Operational": 1,
    //                                        "RS Status": 1,
    //                                        "Receiver 1090 MHz": 1,
    //                                        "Transmitter 1030 MHz": 1,
    //                                        "Transmitter 1090 MHz": 0
    //                                    },
    //                                    {
    //                                        "RS Identification": 8,
    //                                        "RS Operational": 1,
    //                                        "RS Status": 1,
    //                                        "Receiver 1090 MHz": 1,
    //                                        "Transmitter 1030 MHz": 1,
    //                                        "Transmitter 1090 MHz": 0
    //                                    },
    //                                    {
    //                                        "RS Identification": 9,
    //                                        "RS Operational": 1,
    //                                        "RS Status": 1,
    //                                        "Receiver 1090 MHz": 1,
    //                                        "Transmitter 1030 MHz": 1,
    //                                        "Transmitter 1090 MHz": 0
    //                                    },
    //                                    {
    //                                        "RS Identification": 10,
    //                                        "RS Operational": 1,
    //                                        "RS Status": 1,
    //                                        "Receiver 1090 MHz": 1,
    //                                        "Transmitter 1030 MHz": 1,
    //                                        "Transmitter 1090 MHz": 0
    //                                    },
    //                                    {
    //                                        "RS Identification": 11,
    //                                        "RS Operational": 1,
    //                                        "RS Status": 1,
    //                                        "Receiver 1090 MHz": 1,
    //                                        "Transmitter 1030 MHz": 1,
    //                                        "Transmitter 1090 MHz": 0
    //                                    },
    //                                    {
    //                                        "RS Identification": 12,
    //                                        "RS Operational": 1,
    //                                        "RS Status": 1,
    //                                        "Receiver 1090 MHz": 1,
    //                                        "Transmitter 1030 MHz": 1,
    //                                        "Transmitter 1090 MHz": 0
    //                                    },
    //                                    {
    //                                        "RS Identification": 13,
    //                                        "RS Operational": 1,
    //                                        "RS Status": 1,
    //                                        "Receiver 1090 MHz": 1,
    //                                        "Transmitter 1030 MHz": 1,
    //                                        "Transmitter 1090 MHz": 0
    //                                    },
    //                                    {
    //                                        "RS Identification": 14,
    //                                        "RS Operational": 1,
    //                                        "RS Status": 1,
    //                                        "Receiver 1090 MHz": 1,
    //                                        "Transmitter 1030 MHz": 1,
    //                                        "Transmitter 1090 MHz": 0
    //                                    },
    //                                    {
    //                                        "RS Identification": 15,
    //                                        "RS Operational": 1,
    //                                        "RS Status": 1,
    //                                        "Receiver 1090 MHz": 1,
    //                                        "Transmitter 1030 MHz": 1,
    //                                        "Transmitter 1090 MHz": 0
    //                                    },
    //                                    {
    //                                        "RS Identification": 16,
    //                                        "RS Operational": 1,
    //                                        "RS Status": 1,
    //                                        "Receiver 1090 MHz": 1,
    //                                        "Transmitter 1030 MHz": 1,
    //                                        "Transmitter 1090 MHz": 0
    //                                    }
    //                                ]
    //                            },
    //                            "600": {
    //                                "Latitude": 47.49999989656383,
    //                                "Longitude": 13.99999996774894
    //                            },
    //                            "610": {
    //                                "Height": 0.0
    //                            },
    //                            "620": {
    //                                "Undulation": 0.0
    //                            },
    //                            "FSPEC": [
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                false,
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                false,
    //                                false,
    //                                false,
    //                                false,
    //                                false
    //                            ]
    //                        }
    //                    ]
    //                },
    //                "length": 57
    //            }
    //        ]
    //    }

    loginf << "cat019 test: data block" << logendl;

    REQUIRE(json_data->contains("data_blocks"));
    REQUIRE(json_data->at("data_blocks").is_array());
    REQUIRE(json_data->at("data_blocks").size() == 1);

    const json& first_data_block = json_data->at("data_blocks").at(0);

    REQUIRE(first_data_block.contains("category"));
    REQUIRE(first_data_block.at("category") == 19);
    REQUIRE(first_data_block.contains("length"));
    REQUIRE(first_data_block.at("length") == 57);

    loginf << "cat019 test: num records" << logendl;
    REQUIRE(first_data_block.contains("content"));
    REQUIRE(first_data_block.at("content").contains("records"));
    REQUIRE(first_data_block.at("content").at("records").is_array());
    REQUIRE(first_data_block.at("content").at("records").size() == 1);

    const json& record = first_data_block.at("content").at("records").at(0);

    //    ; FSPEC: 0x fd e0

    loginf << "cat019 test: fspec" << logendl;
    REQUIRE(record.at("FSPEC").size() == 2 * 8);

    REQUIRE(record.at("FSPEC") ==
            std::vector<bool>({1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0}));

    //    ; Data Record:
    //    ;  I019/010: =0x 00 03
    //    ;  Data Source Identifier: 0x0003 (SAC=0; SIC=3)

    loginf << "cat019 test: 010" << logendl;
    REQUIRE(record.at("010").at("SAC") == 0);
    REQUIRE(record.at("010").at("SIC") == 3);

    //    ;  I019/000: =0x 02
    //    ;  Message Type: mtp=2 (Periodic status message)

    loginf << "cat019 test: 000" << logendl;
    REQUIRE(record.at("000").at("Message Type") == 2);

    //    ;  I019/140: =0x 41 6f 85
    //    ;  Time of Day: 0x416f85 (4288389; 09:18:23.039 UTC)

    loginf << "cat019 test: 140" << logendl;
    REQUIRE(approximatelyEqual(record.at("140").at("Time of Day"), 33503.0390625, 10e-6));

    //    ;  I019/550: =0x 00
    //    ;  System Status: nogo=0 (operational); ovl=0; tsv=0; ttf=0
    loginf << "cat019 test: 550" << logendl;
    REQUIRE(record.at("550").at("NOGO") == 0);
    REQUIRE(record.at("550").at("OVL") == 0);
    REQUIRE(record.at("550").at("TSV") == 0);
    REQUIRE(record.at("550").at("TTF") == 0);

    //    ;  I019/551: =0x d0
    //    ;  Tracking Processor Detailed Status: 0xd0
    // 11010000
    loginf << "cat019 test: 551" << logendl;
    REQUIRE(record.at("551").at("TP 1a") == 1);
    REQUIRE(record.at("551").at("TP 1b") == 1);
    REQUIRE(record.at("551").at("TP 2a") == 0);
    REQUIRE(record.at("551").at("TP 2b") == 1);
    REQUIRE(record.at("551").at("TP 3a") == 0);
    REQUIRE(record.at("551").at("TP 3b") == 0);
    REQUIRE(record.at("551").at("TP 4a") == 0);
    REQUIRE(record.at("551").at("TP 4b") == 0);

    //    ;  I019/552: =0x 10 01 6c 02  6c 03 6c 04  6c 05 6c 06  6c 07 6c 08
    //    ;            +0x 6c 09 6c 0a  6c 0b 6c 0c  6c 0d 6c 0e  6c 0f 6c 10
    //    ;            +0x 6c
    //    ;  Remote Sensor Detailed Status:
    //    ;   id=1; type=6; status=12
    //    ;   id=2; type=6; status=12
    //    ;   id=3; type=6; status=12
    //    ;   id=4; type=6; status=12
    //    ;   id=5; type=6; status=12
    //    ;   id=6; type=6; status=12
    //    ;   id=7; type=6; status=12
    //    ;   id=8; type=6; status=12
    //    ;   id=9; type=6; status=12
    //    ;   id=10; type=6; status=12
    //    ;   id=11; type=6; status=12
    //    ;   id=12; type=6; status=12
    //    ;   id=13; type=6; status=12
    //    ;   id=14; type=6; status=12
    //    ;   id=15; type=6; status=12
    //    ;   id=16; type=6; status=12

    loginf << "cat019 test: 552" << logendl;
    REQUIRE(record.at("552").at("REP") == 16);
    REQUIRE(record.at("552").at("Remote Sensor Detailed Status").size() == 16);
    // 0110 1100
    // spare 0
    // "Receiver 1090 MHz": 1,
    // "Transmitter 1030 MHz": 1,
    // "Transmitter 1090 MHz": 0
    // "RS Status": 1,
    // "RS Operational": 1,

    REQUIRE(record.at("552").at("Remote Sensor Detailed Status")[0].at("Receiver 1090 MHz") == 1);
    REQUIRE(record.at("552").at("Remote Sensor Detailed Status")[0].at("Transmitter 1030 MHz") ==
            1);
    REQUIRE(record.at("552").at("Remote Sensor Detailed Status")[0].at("Transmitter 1090 MHz") ==
            0);
    REQUIRE(record.at("552").at("Remote Sensor Detailed Status")[0].at("RS Status") == 1);
    REQUIRE(record.at("552").at("Remote Sensor Detailed Status")[0].at("RS Operational") == 1);

    //    ;  I019/600: =0x 10 e3 8e 39  04 fa 4f a5
    //    ;  Position of the MLT System Reference Point:
    //    ;   Latitude: 283348537 (47:30:00.000N)
    //    ;   Longitude: 83513253 (014:00:00.000E)

    loginf << "cat019 test: 600" << logendl;
    REQUIRE(approximatelyEqual(record.at("600").at("Latitude"), 47.50000001862645149231, 10e-10));
    REQUIRE(approximatelyEqual(record.at("600").at("Longitude"), 14.00000000372529029846, 10e-10));

    //    ;  I019/610: =0x 00 00
    //    ;  Height of the MLT System Reference Point: 0.00 mtr
    loginf << "cat019 test: 610" << logendl;
    REQUIRE(record.at("610").at("Height") == 0.0);

    //    ;  I019/620: =0x 00
    //    ;  WGS-84 Undulation: 0 mtr
    loginf << "cat019 test: 620" << logendl;
    REQUIRE(record.at("620").at("Undulation") == 0.0);
}

TEST_CASE("jASTERIX CAT019 1.3", "[jASTERIX CAT019]")
{
    loginf << "cat019 test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, true, true, false);

    //; ASTERIX data block at pos 0: cat=19; len=57
    //      130039fde0000302416f8500d010016c026c036c046c056c066c076c086c096c0a6c0b6c0c6c0d6c0e6c0f6c106c10e38e3904fa4fa5000000

    // echo -n
    // 130039fde0000302416f8500d010016c026c036c046c056c066c076c086c096c0a6c0b6c0c6c0d6c0e6c0f6c106c10e38e3904fa4fa5000000
    // | xxd -r -p > cat019ed1.3.bin

    REQUIRE(jasterix.hasCategory(19));
    std::shared_ptr<jASTERIX::Category> cat019 = jasterix.category(19);
    REQUIRE(cat019->hasEdition("1.3"));
    cat019->setCurrentEdition("1.3");
    cat019->setCurrentMapping("");

    const std::string filename = "cat019ed1.3.bin";

    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));
    REQUIRE(jASTERIX::Files::fileSize(data_path + filename) == 57);

    jasterix.decodeFile(data_path + filename, test_cat019_callback);

    loginf << "cat019 test: end" << logendl;
}
