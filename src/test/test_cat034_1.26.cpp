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

void test_cat034_callback(std::unique_ptr<nlohmann::json> json_data, size_t total_num_bytes, size_t num_frames,
                          size_t num_records, size_t num_errors)
{
    loginf << "cat034 test: decoded " << num_frames << " frames, " << num_records << " records, "
           << num_errors << " errors" << logendl;

    REQUIRE(num_frames == 0);
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);
    //    {
    //        "data_blocks": [
    //            {
    //                "category": 34,
    //                "content": {
    //                    "index": 3,
    //                    "length": 17,
    //                    "records": [
    //                        {
    //                            "000": {
    //                                "Message Type": 2
    //                            },
    //                            "010": {
    //                                "SAC": 0,
    //                                "SIC": 2
    //                            },
    //                            "020": {
    //                                "Sector Number": 90.0
    //                            },
    //                            "030": {
    //                                "Time of Day": 33499.84375
    //                            },
    //                            "050": {
    //                                "COM": {
    //                                    "MSC": 0,
    //                                    "NOGO": 0,
    //                                    "OVL RDP": 0,
    //                                    "OVL XMT": 0,
    //                                    "RDPC": 1,
    //                                    "RDPR": 0,
    //                                    "TSV": 0
    //                                },
    //                                "MDS": {
    //                                    "ANT": 0,
    //                                    "CH-A/B": 2,
    //                                    "DLF": 0,
    //                                    "MSC": 0,
    //                                    "OVL DLF": 0,
    //                                    "OVL SCF": 0,
    //                                    "OVL SUR": 0,
    //                                    "SCF": 1
    //                                },
    //                                "PSR": {
    //                                    "ANT": 0,
    //                                    "CH-A/B": 3,
    //                                    "MSC": 0,
    //                                    "OVL": 0
    //                                },
    //                                "available": [
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    false,
    //                                    true,
    //                                    false,
    //                                    false,
    //                                    true
    //                                ]
    //                            },
    //                            "060": {
    //                                "COM": {
    //                                    "RED-RDP": 0,
    //                                    "RED-XMT": 0
    //                                },
    //                                "MDS": {
    //                                    "CLU": 1,
    //                                    "RED-RAD": 0
    //                                },
    //                                "PSR": {
    //                                    "POL": 0,
    //                                    "RED-RAD": 0,
    //                                    "STC": 0
    //                                },
    //                                "available": [
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    false,
    //                                    true,
    //                                    false,
    //                                    false,
    //                                    true
    //                                ]
    //                            },
    //                            "FSPEC": [
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                false,
    //                                true,
    //                                true,
    //                                false
    //                            ]
    //                        }
    //                    ]
    //                },
    //                "length": 20
    //            }
    //        ]
    //    }

    loginf << "cat034 test: data block" << logendl;

    REQUIRE(json_data->contains("data_blocks"));
    REQUIRE(json_data->at("data_blocks").is_array());
    REQUIRE(json_data->at("data_blocks").size() == 1);

    const json& first_data_block = json_data->at("data_blocks").at(0);

    REQUIRE(first_data_block.contains("category"));
    REQUIRE(first_data_block.at("category") == 34);
    REQUIRE(first_data_block.contains("length"));
    REQUIRE(first_data_block.at("length") == 20);

    loginf << "cat034 test: num records" << logendl;
    REQUIRE(first_data_block.contains("content"));
    REQUIRE(first_data_block.at("content").contains("records"));
    REQUIRE(first_data_block.at("content").at("records").is_array());
    REQUIRE(first_data_block.at("content").at("records").size() == 1);

    const json& record = first_data_block.at("content").at("records").at(0);

    //    ; FSPEC: 0x f6

    loginf << "cat034 test: fspec" << logendl;


    //    ; Data Record:
    //    ;  I034/010: =0x 00 02
    //    ;  Data Source Identifier: 0x0002 (SAC=0; SIC=2)

    loginf << "cat034 test: 010" << logendl;
    REQUIRE(record.at("010").at("SAC") == 0);
    REQUIRE(record.at("010").at("SIC") == 2);

    //    ;  I034/000: =0x 02
    //    ;  Message Type: Sector crossing message

    loginf << "cat034 test: 000" << logendl;
    REQUIRE(record.at("000").at("Message Type") == 2);

    //    ;  I034/030: =0x 41 6d ec
    //    ;  Time of Day: 0x416dec (4287980; 33499.843750 secs; 09:18:19.844 UTC)

    loginf << "cat034 test: 030" << logendl;
    REQUIRE(approximatelyEqual(record.at("030").at("Time of Day"), 33499.84375, 10e-6));

    //    ;  I034/020: =0x 40
    //    ;  Sector Angle: 64 (90.000 degrees)

    loginf << "cat034 test: 020" << logendl;
    REQUIRE(record.at("020").at("Sector Number") == 90.0);

    //    ;  I034/050: 0x 94 40 60 44 00
    loginf << "cat034 test: 050" << logendl;
    REQUIRE(record.at("050").at("COM").is_object());
    REQUIRE(record.at("050").at("PSR").is_object());
    REQUIRE(record.at("050").at("MDS").is_object());

    // 10010100
    // COM 0 0 PSR SSR MDS 0 FX

    //    ;  System Configuration and Status:
    //    ;   Common Part: NOGO=0; RDPC=1; RDPR=0; OVL_RDP=0; OVL_XMT=0; MSC=0; TSV=0
    REQUIRE(record.at("050").at("COM").at("NOGO") == 0);
    REQUIRE(record.at("050").at("COM").at("RDPC") == 1);
    REQUIRE(record.at("050").at("COM").at("RDPR") == 0);
    REQUIRE(record.at("050").at("COM").at("OVL RDP") == 0);
    REQUIRE(record.at("050").at("COM").at("OVL XMT") == 0);
    REQUIRE(record.at("050").at("COM").at("MSC") == 0);
    REQUIRE(record.at("050").at("COM").at("TSV") == 0);

    //    ;   PSR Sensor: ANT=0; CH-A/B=3; OVL=0; MSC=0
    REQUIRE(record.at("050").at("PSR").at("ANT") == 0);
    REQUIRE(record.at("050").at("PSR").at("CH-A/B") == 3);
    REQUIRE(record.at("050").at("PSR").at("OVL") == 0);
    REQUIRE(record.at("050").at("PSR").at("MSC") == 0);

    //    ;   Mode S Sensor: ANT=0; CH-A/B=2; OVL_SUR=0; MSC=0; SCF=1; DLF=0; OVL_SCF=0; OVL_DLF=0
    REQUIRE(record.at("050").at("MDS").at("ANT") == 0);
    REQUIRE(record.at("050").at("MDS").at("CH-A/B") == 2);
    REQUIRE(record.at("050").at("MDS").at("OVL SUR") == 0);
    REQUIRE(record.at("050").at("MDS").at("MSC") == 0);
    REQUIRE(record.at("050").at("MDS").at("SCF") == 1);
    REQUIRE(record.at("050").at("MDS").at("DLF") == 0);
    REQUIRE(record.at("050").at("MDS").at("OVL SCF") == 0);
    REQUIRE(record.at("050").at("MDS").at("OVL DLF") == 0);

    //    ;  I034/060: 0x 94 00 00 10
    loginf << "cat034 test: 060" << logendl;

    // 10010100
    // COM 0 0 PSR SSR MDS 0 FX
    REQUIRE(record.at("060").at("COM").is_object());
    REQUIRE(record.at("060").at("PSR").is_object());
    REQUIRE(record.at("060").at("MDS").is_object());

    //    ;  System Processing Mode:
    //    ;   Common Part: RED_RDP=0; RED_XMT=0
    REQUIRE(record.at("060").at("COM").at("RED-RDP") == 0);
    REQUIRE(record.at("060").at("COM").at("RED-XMT") == 0);

    //    ;   PSR Sensor: POL=0; RED_RAD=0; STC=0
    REQUIRE(record.at("060").at("PSR").at("POL") == 0);
    REQUIRE(record.at("060").at("PSR").at("RED-RAD") == 0);
    REQUIRE(record.at("060").at("PSR").at("STC") == 0);

    //    ;   Mode S Sensor: RED_RAD=0; CLU=1
    REQUIRE(record.at("060").at("MDS").at("RED-RAD") == 0);
    REQUIRE(record.at("060").at("MDS").at("CLU") == 1);

    //     [--:--:--.---] M 09:18:19.844 -- 0x0002 A1 S:SCT ----- A-   90.000
}

TEST_CASE("jASTERIX CAT034 1.26", "[jASTERIX CAT034]")
{
    loginf << "cat034 test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, true, true, false);

    //    ; ASTERIX data block at pos 0: cat=34; len=20
    //      220014f6000202416dec40944060440094000010

    // echo -n 220014f6000202416dec40944060440094000010 | xxd -r -p > cat034ed1.26.bin

    REQUIRE(jasterix.hasCategory(34));
    std::shared_ptr<jASTERIX::Category> cat034 = jasterix.category(34);
    REQUIRE(cat034->hasEdition("1.26"));
    cat034->setCurrentEdition("1.26");
    cat034->setCurrentMapping("");

    const std::string filename = "cat034ed1.26.bin";

    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));
    REQUIRE(jASTERIX::Files::fileSize(data_path + filename) == 20);

    jasterix.decodeFile(data_path + filename, test_cat034_callback);

    loginf << "cat034 test: end" << logendl;
}
