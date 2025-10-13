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

void test_cat021_callback(std::unique_ptr<nlohmann::json> json_data, size_t num_frames,
                          size_t num_records, size_t num_errors)
{
    loginf << "cat021 test: decoded " << num_frames << " frames, " << num_records << " records, "
           << num_errors << " errors" << logendl;

    REQUIRE(num_frames == 0);
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

    //    {
    //        "data_blocks": [
    //            {
    //                "category": 21,
    //                "content": {
    //                    "index": 3,
    //                    "length": 46,
    //                    "records": [
    //                        {
    //                            "010": {
    //                                "SAC": 0,
    //                                "SIC": 3
    //                            },
    //                            "015": {
    //                                "Service Identification": 0
    //                            },
    //                            "016": {
    //                                "RP": 2.0
    //                            },
    //                            "040": {
    //                                "ARC": 0,
    //                                "ATP": 0,
    //                                "CL": 0,
    //                                "DCR": 0,
    //                                "FX": 1,
    //                                "FX2": 0,
    //                                "GBS": 0,
    //                                "RAB": 0,
    //                                "RC": 0,
    //                                "SAA": 1,
    //                                "SIM": 0,
    //                                "TST": 0
    //                            },
    //                            "070": {
    //                                "Mode-3/A": 7106
    //                            },
    //                            "073": {
    //                                "Time of Message Reception for Position": 33502.8828125
    //                            },
    //                            "075": {
    //                                "Time of Message Reception of Velocity": 33502.46875
    //                            },
    //                            "077": {
    //                                "Time Of ASTERIX Report Transmission": 33503.1328125
    //                            },
    //                            "080": {
    //                                "Target Address": 1723237
    //                            },
    //                            "090": {
    //                                "FX": 0,
    //                                "NUCp or NIC": 7,
    //                                "NUCr or NACv": 0
    //                            },
    //                            "130": {
    //                                "Latitude": 46.844196461660005,
    //                                "Longitude": 12.29852793351
    //                            },
    //                            "140": {
    //                                "Geometric Height": 34750.0
    //                            },
    //                            "145": {
    //                                "Flight Level": 350.0
    //                            },
    //                            "161": {
    //                                "Track Number": 1375
    //                            },
    //                            "170": {
    //                                "Target Identification": "EZS14ZH "
    //                            },
    //                            "200": {
    //                                "ICF": 0,
    //                                "LNAV": 0,
    //                                "PS": 0,
    //                                "SS": 0
    //                            },
    //                            "210": {
    //                                "LTT": 2,
    //                                "VN": 0,
    //                                "VNS": 0
    //                            },
    //                            "FSPEC": [
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                false,
    //                                true,
    //                                false,
    //                                true,
    //                                false,
    //                                false,
    //                                false,
    //                                true,
    //                                true,
    //                                false,
    //                                true,
    //                                true,
    //                                false,
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                false,
    //                                true,
    //                                true,
    //                                false,
    //                                true,
    //                                false,
    //                                false,
    //                                false,
    //                                false,
    //                                true,
    //                                true,
    //                                true,
    //                                false,
    //                                false,
    //                                false,
    //                                false,
    //                                false,
    //                                true,
    //                                false
    //                            ]
    //                        }
    //                    ]
    //                },
    //                "length": 49
    //            }
    //        ]
    //    }

    loginf << "cat021 test: data block" << logendl;

    REQUIRE(json_data->contains("data_blocks"));
    REQUIRE(json_data->at("data_blocks").is_array());
    REQUIRE(json_data->at("data_blocks").size() == 1);

    const json& first_data_block = json_data->at("data_blocks").at(0);

    REQUIRE(first_data_block.contains("category"));
    REQUIRE(first_data_block.at("category") == 21);
    REQUIRE(first_data_block.contains("length"));
    REQUIRE(first_data_block.at("length") == 49);

    loginf << "cat021 test: num records" << logendl;
    REQUIRE(first_data_block.contains("content"));
    REQUIRE(first_data_block.at("content").contains("records"));
    REQUIRE(first_data_block.at("content").at("records").is_array());
    REQUIRE(first_data_block.at("content").at("records").size() == 1);

    const json& record = first_data_block.at("content").at("records").at(0);

    //    ; FSPEC: 0x f5 1b 7b 43 82
    // 1111010100011011011110110100001110000010

    loginf << "cat021 test: fspec" << logendl;
    REQUIRE(record.at("FSPEC").size() == 5 * 8);

    REQUIRE(record.at("FSPEC") ==
            std::vector<bool>({1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1,
                               1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0}));

    //    ;  I021/010: =0x 00 03
    //    ;  Data Source Identifier: 0x0003 (SAC=0; SIC=3)

    loginf << "cat021 test: 010" << logendl;
    REQUIRE(record.at("010").at("SAC") == 0);
    REQUIRE(record.at("010").at("SIC") == 3);

    //    ;  I021/040: =0x 01 08
    //    ;  Target Report Descriptor:
    //    ;   atp=0 (24-Bit ICAO address); arc=0 (25 ft); rc=0; rab=0
    //    ;   dcr=0; gbs=0; sim=0; tst=0; saa=1; cl=0 (Report valid)

    loginf << "cat021 test: 040" << logendl;
    REQUIRE(record.at("040").at("ATP") == 0);
    REQUIRE(record.at("040").at("ARC") == 0);
    REQUIRE(record.at("040").at("RC") == 0);
    REQUIRE(record.at("040").at("RAB") == 0);
    REQUIRE(record.at("040").at("FX") == 1);

    REQUIRE(record.at("040").at("GBS") == 0);
    REQUIRE(record.at("040").at("SIM") == 0);
    REQUIRE(record.at("040").at("TST") == 0);
    REQUIRE(record.at("040").at("SAA") == 1);
    REQUIRE(record.at("040").at("CL") == 0);
    REQUIRE(record.at("040").at("FX2") == 0);

    //    ;  I021/161: =0x 05 5f
    //    ;  Track Number: tn=1375

    loginf << "cat021 test: 161" << logendl;
    REQUIRE(record.at("161").at("Track Number") == 1375);

    //    ;  I021/015: =0x 00
    //    ;  Service Identification: 0

    loginf << "cat021 test: 015" << logendl;
    REQUIRE(record.at("015").at("Service Identification") == 0);

    //    ;  I021/130: =0x 21 4f ba 08  be e1
    //    ;  Position in WGS-84 Co-ordinates:
    //    ;   lat=2183098 (46:50:39.124N); lon=573153 (012:17:54.705E)

    loginf << "cat021 test: 130" << logendl;
    REQUIRE(approximatelyEqual(record.at("130").at("Latitude"), 46.84420108795166015625, 10e-10));
    REQUIRE(approximatelyEqual(record.at("130").at("Longitude"), 12.29852914810180664063, 10e-10));

    //    ;  I021/080: =0x 1a 4b 65
    //    ;  Target Address: 0x1a4b65 (1723237)

    loginf << "cat021 test: 080" << logendl;
    REQUIRE(record.at("080").at("Target Address") == 1723237);

    //    ;  I021/073: =0x 41 6f 71
    //    ;  Time of Message Reception for Position: 0x416f71 (4288369; 09:18:22.883 UTC)

    loginf << "cat021 test: 073" << logendl;
    REQUIRE(approximatelyEqual(record.at("073").at("Time of Message Reception for Position"),
                               33502.8828125, 10e-6));

    //    ;  I021/075: =0x 41 6f 3c
    //    ;  Time of Message Reception for Velocity: 0x416f3c (4288316; 09:18:22.469 UTC)

    loginf << "cat021 test: 075" << logendl;
    REQUIRE(approximatelyEqual(record.at("075").at("Time of Message Reception of Velocity"),
                               33502.46875, 10e-6));

    //    ;  I021/140: =0x 15 b8
    //    ;  Geometric Height: 5560 (34750.00 ft)

    loginf << "cat021 test: 140" << logendl;
    REQUIRE(record.at("140").at("Geometric Height") == 34750.00);

    //    ;  I021/090: =0x 0e
    //    ;  Quality Indicators:
    //    ;   NUCr_or_NACv=0; NUCp_or_NIC=7

    loginf << "cat021 test: 090" << logendl;
    REQUIRE(record.at("090").at("NUCr or NACv") == 0);
    REQUIRE(record.at("090").at("NUCp or NIC") == 7);

    //    ;  I021/210: =0x 02
    //    ;  MOPS Version: vns=0; vn=0 (DO-260); ltt=2 (1090 ES)

    loginf << "cat021 test: 210" << logendl;
    REQUIRE(record.at("210").at("VNS") == 0);
    REQUIRE(record.at("210").at("VN") == 0);
    REQUIRE(record.at("210").at("LTT") == 2);

    //    ;  I021/070: =0x 0e 46
    //    ;  Mode 3/A Code: 07106

    loginf << "cat021 test: 070" << logendl;
    REQUIRE(record.at("070").at("Mode-3/A") == 7106);

    //    ;  I021/145: =0x 05 78
    //    ;  Flight Level: 1400 (350.00 FL)

    loginf << "cat021 test: 145" << logendl;
    REQUIRE(record.at("145").at("Flight Level") == 350.00);

    //    ;  I021/200: =0x 00
    //    ;  Target Status: icf=0; lnav=0; ps=0; ss=0

    loginf << "cat021 test: 200" << logendl;
    REQUIRE(record.at("200").at("ICF") == 0);
    REQUIRE(record.at("200").at("LNAV") == 0);
    REQUIRE(record.at("200").at("PS") == 0);
    REQUIRE(record.at("200").at("SS") == 0);

    //    ;  I021/077: =0x 41 6f 91
    //    ;  Time of ASTERIX Report Transmission: 0x416f91 (4288401; 09:18:23.133 UTC)

    loginf << "cat021 test: 077" << logendl;
    REQUIRE(approximatelyEqual(record.at("077").at("Time Of ASTERIX Report Transmission"),
                               33503.1328125, 10e-6));

    //    ;  I021/170: =0x 15 a4 f1 d1  a2 20
    //    ;  Target Identification: idt=[EZS14ZH ]

    loginf << "cat021 test: 170" << logendl;
    REQUIRE(record.at("170").at("Target Identification") == "EZS14ZH ");

    //    ;  I021/016: =0x 04
    //    ;  Service Management: rp=4 (2.0 sec)

    loginf << "cat021 test: 016" << logendl;
    REQUIRE(record.at("016").at("RP") == 2.0);
}

TEST_CASE("jASTERIX CAT021 2.1", "[jASTERIX CAT021]")
{
    loginf << "cat021 test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, true, true, false);

    //; ASTERIX data block at pos 0: cat=21; len=49
    //  150031f51b7b438200030108055f00214fba08bee11a4b65416f71416f3c15b80e020e46057800416f9115a4f1d1a22004

    // echo -n
    // 150031f51b7b438200030108055f00214fba08bee11a4b65416f71416f3c15b80e020e46057800416f9115a4f1d1a22004
    // | xxd -r -p > cat021ed2.1.bin

    REQUIRE(jasterix.hasCategory(21));
    std::shared_ptr<jASTERIX::Category> cat021 = jasterix.category(21);
    REQUIRE(cat021->hasEdition("2.1"));
    cat021->setCurrentEdition("2.1");
    cat021->setCurrentMapping("");

    const std::string filename = "cat021ed2.1.bin";

    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));
    REQUIRE(jASTERIX::Files::fileSize(data_path + filename) == 49);

    jasterix.decodeFile(data_path + filename, test_cat021_callback);

    loginf << "cat021 test: end" << logendl;
}
