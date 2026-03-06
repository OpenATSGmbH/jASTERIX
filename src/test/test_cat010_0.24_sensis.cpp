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

void test_cat010_sensis_callback(std::unique_ptr<nlohmann::json> json_data, size_t total_num_bytes, size_t num_frames,
                          size_t num_records, size_t num_errors)
{
    loginf << "cat010 sensis test: decoded " << num_frames << " frames, " << num_records << " records, "
           << num_errors << " errors" << logendl;

    REQUIRE(num_frames == 0);
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

    //    {
    //        "data_blocks": [
    //            {
    //                "category": 10,
    //                "content": {
    //                    "index": 3,
    //                    "length": 40,
    //                    "records": [
    //                        {
    //                            "000": {
    //                                "Message Type": 1
    //                            },
    //                            "010": {
    //                                "SAC": 0,
    //                                "SIC": 0
    //                            },
    //                            "020": {
    //                                "CHN": 0,
    //                                "CRT": 1,
    //                                "DCR": 0,
    //                                "FX": 1,
    //                                "FX2": 1,
    //                                "GBS": 0,
    //                                "LOP": 0,
    //                                "RAB": 0,
    //                                "SIM": 0,
    //                                "SPI": 0,
    //                                "TOT": 0,
    //                                "TST": 0,
    //                                "TYP": 1
    //                            },
    //                            "041": {
    //                                "Latitude": 25.6226482660952,
    //                                "Longitude": 22.1754983026788
    //                            },
    //                            "042": {
    //                                "X": -1423.0,
    //                                "Y": 1368.0
    //                            },
    //                            "091": {
    //                                "Measured Height": 0.0
    //                            },
    //                            "140": {
    //                                "Time-of-Day": 84080.484375
    //                            },
    //                            "161": {
    //                                "TRACK NUMBER": 922
    //                            },
    //                            "170": {
    //                                "CNF": 0,
    //                                "CST": 0,
    //                                "FX": 0,
    //                                "MAH": 0,
    //                                "STH": 1,
    //                                "TCC": 0,
    //                                "TRE": 0
    //                            },
    //                            "202": {
    //                                "Vx": 4088,
    //                                "Vy": 1
    //                            },
    //                            "220": {
    //                                "Target Address": 4672673
    //                            },
    //                            "500": {
    //                                "SDP-x": 2.5,
    //                                "SDP-xy": -1.0,
    //                                "SDP-y": 3.25
    //                            },
    //                            "FSPEC": [
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                false,
    //                                true,
    //                                true,
    //                                false,
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
    //                                false,
    //                                false,
    //                                false,
    //                                true,
    //                                true,
    //                                false,
    //                                false,
    //                                false,
    //                                false,
    //                                false,
    //                                false,
    //                                false
    //                            ],
    //                            "index": 3,
    //                            "length": 40
    //                        }
    //                    ]
    //                },
    //                "length": 43
    //            }
    //        ]
    //    }


    loginf << "cat010 sensis test: data block" << logendl;

    REQUIRE(json_data->contains("data_blocks"));
    REQUIRE(json_data->at("data_blocks").is_array());
    REQUIRE(json_data->at("data_blocks").size() == 1);

    const json& first_data_block = json_data->at("data_blocks").at(0);

    REQUIRE(first_data_block.contains("category"));
    REQUIRE(first_data_block.at("category") == 10);
    REQUIRE(first_data_block.contains("length"));
    REQUIRE(first_data_block.at("length") == 43);

    loginf << "cat010 sensis test: num records" << logendl;
    REQUIRE(first_data_block.contains("content"));
    REQUIRE(first_data_block.at("content").contains("records"));
    REQUIRE(first_data_block.at("content").at("records").is_array());
    REQUIRE(first_data_block.at("content").at("records").size() == 1);

    const json& record = first_data_block.at("content").at("records").at(0);

//    ; FSPEC: 0x fb 75 11 80
//    ; Data Record:
//    ;  I010/010: =0x 00 00
//    ;  Data Source Identifier: 0x0000 (SAC=0; SIC=0)
//    ;  I010/000: =0x 01
//    ;  Message Type: 1 (Target Report)
//    ;  I010/020: =0x 23 01 00
//    ;  Target Report Descriptor: TYP='Mode S multilateration' CHN-1 CRT LOP=0 TOT=0
//    ;  I010/140: =0x a4 38 3e
//    ;  Time of Day: tod=0xa4383e (10762302; 23:21:20.484 UTC)
//    ;  I010/041: =0x 12 38 75 f5  0f c4 ed 20
//    ;  Position in WGS-84 Co-ordinates:
//    ;   lat=305690101 (25:37:21.534N); lon=264564000 (022:10:31.794E)
//    ;  I010/042: =0x fa 71 05 58
//    ;  Position in Cartesian Coordinates: x=-1423 mtr; y=1368 mtr
//    ;  I010/202: =0x ff 80 01
//    ;  Calculated Track Velocity in Cartesian Co-ordinates: vx=-8.00 m/s; vy=1.00 m/s
//    ;  I010/161: =0x 03 9a
//    ;  Track Number: tno=922
//    ;  I010/170: =0x 02
//    ;  Track Status:
//    ;   CON CST=0 TCC=0 STH=1
//    ;  I010/220: =0x 47 4c a1
//    ;  Aircraft Address: 0x474ca1 (4672673)
//    ;  I010/091: =0x 00 00
//    ;  Measured Height: 0.00 FL
//    ;  I010/500: =0x 0a 0d ff fc
//    ;  Standard Deviation in Position: sx=2.50 mtr; sy=3.25 mtr; sxy=-1.00 mtr**2
//     [--:--:--.---] D 23:21:20.484 -- 0x0000 A- M:MDS #0922 XY   -0.768    0.739 S:0x474ca1            -:----- --- C:----    -- ;


    //    ; FSPEC: 0x fb 75 11 80
    //    11111011 01110101 00010001 10000000

    loginf << "cat010 sensis test: fspec" << logendl;


    //    ;  I010/010: =0x 00 00
    //    ;  Data Source Identifier: 0x0000 (SAC=0; SIC=0)

    loginf << "cat010 sensis test: 010" << logendl;
    REQUIRE(record.at("010").at("SAC") == 0);
    REQUIRE(record.at("010").at("SIC") == 0);

    //    ;  I010/000: =0x 01
    //    ;  Message Type: 1 (Target Report)

    loginf << "cat010 sensis test: 000" << logendl;
    REQUIRE(record.at("000").at("Message Type") == 1);

    //    ;  I010/020: =0x 23 01 00
    //    ;  Target Report Descriptor: TYP='Mode S multilateration' CHN-1 CRT LOP=0 TOT=0

    loginf << "cat010 sensis test: 020" << logendl;
    REQUIRE(record.at("020").at("TYP") == 1);
    REQUIRE(record.at("020").at("DCR") == 0);
    REQUIRE(record.at("020").at("CHN") == 0);
    REQUIRE(record.at("020").at("GBS") == 0);
    REQUIRE(record.at("020").at("CRT") == 1);

    REQUIRE(record.at("020").at("SIM") == 0);
    REQUIRE(record.at("020").at("TST") == 0);
    REQUIRE(record.at("020").at("RAB") == 0);
    REQUIRE(record.at("020").at("LOP") == 0);
    REQUIRE(record.at("020").at("TOT") == 0);

    REQUIRE(record.at("020").at("SPI") == 0);

    //    ;  I010/140: =0x a4 38 3e
    //    ;  Time of Day: tod=0xa4383e (10762302; 23:21:20.484 UTC)

    loginf << "cat010 sensis test: 140" << logendl;
    REQUIRE(approximatelyEqual(record.at("140").at("Time-of-Day"), 84080.484375, 10e-6));

    //    ;  I010/041: =0x 12 38 75 f5  0f c4 ed 20
    //    ;  Position in WGS-84 Co-ordinates:
    //    ;   lat=305690101 (25:37:21.534N); lon=264564000 (022:10:31.794E)

    loginf << "cat010 sensis test: 41" << logendl;
    REQUIRE(approximatelyEqual(record.at("041").at("Latitude"), 25.62264827080070972443, 10e-10));
    REQUIRE(approximatelyEqual(record.at("041").at("Longitude"), 22.1754983067512512207, 10e-10));

    //                                "Latitude": 25.6226482660952,
    //                                "Longitude": 22.1754983026788


    //    ;  I010/042: =0x fa 71 05 58
    //    ;  Position in Cartesian Coordinates: x=-1423 mtr; y=1368 mtr

    loginf << "cat010 sensis test: 042" << logendl;
    REQUIRE(record.at("042").at("X") == -1423.0);
    REQUIRE(record.at("042").at("Y") == 1368.0);

    //    ;  I010/202: =0x ff 80 01
    //    ;  Calculated Track Velocity in Cartesian Co-ordinates: vx=-8.00 m/s; vy=1.00 m/s

    //                            "202": {
    //                                "Vx": 4088,
    //                                "Vy": 1
    //                            },

    loginf << "cat010 sensis test: 202" << logendl;
    REQUIRE(record.at("202").at("Vx") == -8.0);
    REQUIRE(record.at("202").at("Vy") == 1.0);

    //    ;  I010/161: =0x 03 9a
    //    ;  Track Number: tno=922

    loginf << "cat010 sensis test: 161" << logendl;
    REQUIRE(record.at("161").at("TRACK NUMBER") == 922);

    //    ;  I010/170: =0x 02
    //    ;  Track Status:
    //    ;   CON CST=0 TCC=0 STH=1

    loginf << "cat010 sensis test: 170" << logendl;
    REQUIRE(record.at("170").at("CNF") == 0);
    REQUIRE(record.at("170").at("TRE") == 0);
    REQUIRE(record.at("170").at("CST") == 0);
    REQUIRE(record.at("170").at("MAH") == 0);
    REQUIRE(record.at("170").at("TCC") == 0);
    REQUIRE(record.at("170").at("STH") == 1);

    //    ;  I010/220: =0x 47 4c a1
    //    ;  Aircraft Address: 0x474ca1 (4672673)

    loginf << "cat010 sensis test: 220" << logendl;
    REQUIRE(record.at("220").at("Target Address") == 4672673);

    //    ;  I010/091: =0x 00 00
    //    ;  Measured Height: 0.00 FL

    loginf << "cat010 sensis test: 210" << logendl;
    REQUIRE(record.at("091").at("Measured Height") == 0.0);

    //    ;  I010/500: =0x 0a 0d ff fc
    //    ;  Standard Deviation in Position: sx=2.50 mtr; sy=3.25 mtr; sxy=-1.00 mtr**2

    loginf << "cat010 sensis test: 500" << logendl;
    REQUIRE(record.at("500").at("SDP-x") == 2.5);
    REQUIRE(record.at("500").at("SDP-xy") == -1.00);
    REQUIRE(record.at("500").at("SDP-y") == 3.25);

}

TEST_CASE("jASTERIX CAT010 0.24 Sensis", "[jASTERIX CAT010 024 Sensis]")
{
    loginf << "cat010 sensis test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, true, true, false);

    // 0a002bfb751180000001230100a4383e123875f50fc4ed20fa710558ff8001039a02474ca100000a0dfffc
    // echo -n  0a002bfb751180000001230100a4383e123875f50fc4ed20fa710558ff8001039a02474ca100000a0dfffc | xxd -r -p > cat010ed0.24_sensis.bin

    REQUIRE(jasterix.hasCategory(10));
    std::shared_ptr<jASTERIX::Category> cat010 = jasterix.category(10);
    REQUIRE(cat010->hasEdition("0.24_sensis"));
    cat010->setCurrentEdition("0.24_sensis");
    cat010->setCurrentMapping("");

    const std::string filename = "cat010ed0.24_sensis.bin";

    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));
    REQUIRE(jASTERIX::Files::fileSize(data_path + filename) == 43);

    jasterix.decodeFile(data_path + filename, test_cat010_sensis_callback);

    loginf << "cat010 sensis ed 0.31 test: end" << logendl;
}
