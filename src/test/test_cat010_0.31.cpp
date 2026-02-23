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

void test_cat010_callback(std::unique_ptr<nlohmann::json> json_data, size_t num_frames,
                          size_t num_records, size_t num_errors)
{
    loginf << "cat010 test: decoded " << num_frames << " frames, " << num_records << " records, "
           << num_errors << " errors" << logendl;

    REQUIRE(num_frames == 0);
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

//    {
//         "data_blocks": [
//             {
//                 "category": 10,
//                 "content": {
//                     "index": 3,
//                     "length": 38,
//                     "records": [
//                         {
//                             "000": {
//                                 "Message Type": 1
//                             },
//                             "010": {
//                                 "SAC": 0,
//                                 "SIC": 1
//                             },
//                             "020": {
//                                 "CHN": 0,
//                                 "CRT": 0,
//                                 "DCR": 0,
//                                 "FX": 1,
//                                 "FX2": 0,
//                                 "GBS": 0,
//                                 "LOP": 0,
//                                 "RAB": 0,
//                                 "SIM": 0,
//                                 "TOT": 0,
//                                 "TST": 0,
//                                 "TYP": 3
//                             },
//                             "040": {
//                                 "RHO": 1588.0,
//                                 "THETA": 189.50866690594
//                             },
//                             "042": {
//                                 "X": -267.0,
//                                 "Y": -1566.0
//                             },
//                             "140": {
//                                 "Time-of-Day": 24693.140625
//                             },
//                             "161": {
//                                 "TRACK NUMBER": 4
//                             },
//                             "170": {
//                                 "CNF": 0,
//                                 "CST": 0,
//                                 "DOU": 0,
//                                 "FX": 1,
//                                 "FX2": 1,
//                                 "GHO": 0,
//                                 "MAH": 0,
//                                 "MRS": 0,
//                                 "STH": 1,
//                                 "TCC": 0,
//                                 "TOM": 3,
//                                 "TRE": 0
//                             },
//                             "200": {
//                                 "Ground Speed": 0.000244140625,
//                                 "Track Angle": 267.275390625
//                             },
//                             "202": {
//                                 "Vx": -0.5,
//                                 "Vy": 0.0
//                             },
//                             "210": {
//                                 "Ax": -1.0,
//                                 "Ay": -0.25
//                             },
//                             "270": {
//                                 "FX": 1,
//                                 "FX2": 1,
//                                 "LENGTH": 27.0,
//                                 "ORIENTATION": 267.1875,
//                                 "WIDTH": 40.0
//                             },
//                             "FSPEC": [
//                                 true,
//                                 true,
//                                 true,
//                                 true,
//                                 false,
//                                 true,
//                                 true,
//                                 true,
//                                 true,
//                                 true,
//                                 true,
//                                 true,
//                                 false,
//                                 false,
//                                 false,
//                                 true,
//                                 false,
//                                 false,
//                                 false,
//                                 false,
//                                 true,
//                                 false,
//                                 false,
//                                 true,
//                                 false,
//                                 false,
//                                 false,
//                                 true,
//                                 false,
//                                 false,
//                                 false,
//                                 false
//                             ],
//                             "index": 3,
//                             "length": 38
//                         }
//                     ]
//                 },
//                 "length": 41
//             }
//         ]
//     }

    loginf << "cat010 test: data block" << logendl;

    REQUIRE(json_data->contains("data_blocks"));
    REQUIRE(json_data->at("data_blocks").is_array());
    REQUIRE(json_data->at("data_blocks").size() == 1);

    const json& first_data_block = json_data->at("data_blocks").at(0);

    REQUIRE(first_data_block.contains("category"));
    REQUIRE(first_data_block.at("category") == 10);
    REQUIRE(first_data_block.contains("length"));
    REQUIRE(first_data_block.at("length") == 41);

    loginf << "cat010 test: num records" << logendl;
    REQUIRE(first_data_block.contains("content"));
    REQUIRE(first_data_block.at("content").contains("records"));
    REQUIRE(first_data_block.at("content").at("records").is_array());
    REQUIRE(first_data_block.at("content").at("records").size() == 1);

    const json& record = first_data_block.at("content").at("records").at(0);

    //    ; FSPEC: 0x f7 f1 09 10
    //    11110111 11110001 00001001 00010000

    loginf << "cat010 test: fspec" << logendl;
    REQUIRE(record.at("FSPEC").size() == 4 * 8);

    REQUIRE(record.at("FSPEC") ==
            std::vector<bool>({1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1,
                               0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0}));

    //    ;  I010/010: =0x 00 01
    //    ;  Data Source Identifier: 0x0001 (SAC=0; SIC=1)

    loginf << "cat010 test: 010" << logendl;
    REQUIRE(record.at("010").at("SAC") == 0);
    REQUIRE(record.at("010").at("SIC") == 1);

    //    ;  I010/000: =0x 01
    //    ;  Message Type: 1 (Target Report)

    loginf << "cat010 test: 000" << logendl;
    REQUIRE(record.at("000").at("Message Type") == 1);

    //    ;  I010/020: =0x 61 00
    //    ;  Target Report Descriptor: TYP='PSR' CHN-1 LOP=0 TOT=0
    //
    //    ; NOTE: this case shows a poor encoding since the first extent should have not been sent

    loginf << "cat010 test: 020" << logendl;
    REQUIRE(record.at("020").at("TYP") == 3);
    REQUIRE(record.at("020").at("DCR") == 0);
    REQUIRE(record.at("020").at("CHN") == 0);
    REQUIRE(record.at("020").at("GBS") == 0);
    REQUIRE(record.at("020").at("CRT") == 0);
    REQUIRE(record.at("020").at("FX") == 1);

    REQUIRE(record.at("020").at("SIM") == 0);
    REQUIRE(record.at("020").at("TST") == 0);
    REQUIRE(record.at("020").at("RAB") == 0);
    REQUIRE(record.at("020").at("LOP") == 0);
    REQUIRE(record.at("020").at("TOT") == 0);
    REQUIRE(record.at("020").at("FX2") == 0);

    //    ;  I010/040: =0x 30 3a 92
    //    ;  Measured Position: srg=1588 (0.857 nmi); azm=34499 (189.509 deg)

    loginf << "cat010 test: 040" << logendl;
    REQUIRE(record.at("040").at("RHO") == 1588.0);
    REQUIRE(approximatelyEqual(record.at("040").at("THETA"), 189.5086669921875, 10e-11));

    //    ;  I010/042: =0x fe f5 f9 e2
    //    ;  Calculated Position: x=-267 mtr; y=-1566 mtr

    loginf << "cat010 test: 042" << logendl;
    REQUIRE(record.at("042").at("X") == -267.0);
    REQUIRE(record.at("042").at("Y") == -1566.0);

    //    ;  I010/140: =0x 30 3a 92
    //    ;  Time of Day: tod=0x303a92 (3160722; 06:51:33.141 UTC)

    loginf << "cat010 test: 140" << logendl;
    REQUIRE(approximatelyEqual(record.at("140").at("Time-of-Day"), 24693.140625, 10e-6));

    //    ;  I010/161: =0x 00 04
    //    ;  Track Number: tno=4

    loginf << "cat010 test: 161" << logendl;
    REQUIRE(record.at("161").at("TRACK NUMBER") == 4);

    //    ;  I010/170: =0x 03 c1 00
    //    ;  Track Status:
    //    ;   CON CST=0 TCC=0 STH=1
    //    ;   TOM=3 DOU=0 mrs=0
    //
    //    ; NOTE: this case shows a poor encoding since the second extent should have not been sent

    loginf << "cat010 test: 170" << logendl;
    REQUIRE(record.at("170").at("CNF") == 0);
    REQUIRE(record.at("170").at("TRE") == 0);
    REQUIRE(record.at("170").at("CST") == 0);
    REQUIRE(record.at("170").at("MAH") == 0);
    REQUIRE(record.at("170").at("TCC") == 0);
    REQUIRE(record.at("170").at("STH") == 1);
    REQUIRE(record.at("170").at("FX") == 1);

    REQUIRE(record.at("170").at("TOM") == 3);
    REQUIRE(record.at("170").at("DOU") == 0);
    REQUIRE(record.at("170").at("MRS") == 0);
    REQUIRE(record.at("170").at("FX2") == 1);

    REQUIRE(record.at("170").at("GHO") == 0);

    //    ;  I010/200: =0x 00 04 be 10
    //    ;  Calculated Track Velocity: spd=4 (0.879 kts); hdg=48656 (267.275 deg)

    loginf << "cat010 test: 200" << logendl;
    REQUIRE(approximatelyEqual(record.at("200").at("Ground Speed"), 0.878906248, 10e-6));
    REQUIRE(approximatelyEqual(record.at("200").at("Track Angle"), 267.275390625, 10e-10));

    //    ;  I010/202: =0x ff fe 00 00
    //    ;  Calculated Track Velocity in Cartesian Co-ordinates: vx=-0.50 m/s; vy=0.00 m/s

    loginf << "cat010 test: 202" << logendl;
    REQUIRE(record.at("202").at("Vx") == -0.5);
    REQUIRE(record.at("202").at("Vy") == 0.0);

    //    ;  I010/210: =0x fc ff
    //    ;  Calculated Acceleration: ax=-1.00 [m/s2]; ay=-0.25 [m/s2]

    loginf << "cat010 test: 210" << logendl;
    REQUIRE(record.at("210").at("Ax") == -1.0);
    REQUIRE(record.at("210").at("Ay") == -0.25);

    //    ;  I010/270: =0x 37 bf 50
    //    ;  Target Size and Orientation:
    //    ;  length=95
    //    ;  orientation=40

    loginf << "cat010 test: 210" << logendl;
    REQUIRE(record.at("270").at("LENGTH") == 27.0);
    REQUIRE(record.at("270").at("FX") == 1);

    REQUIRE(approximatelyEqual(record.at("270").at("ORIENTATION"), 267.1875, 10e-4));
    REQUIRE(record.at("270").at("FX2") == 1);

    REQUIRE(record.at("270").at("WIDTH") == 40.0);
}

TEST_CASE("jASTERIX CAT010 0.31", "[jASTERIX CAT010]")
{
    loginf << "cat010 test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, true, true, false);

    // 0a0029f7f109100001016100303a92063486c3fef5f9e20004be10fffe0000000403c10037bf50fcff
    // echo -n
    // 0a0029f7f109100001016100303a92063486c3fef5f9e20004be10fffe0000000403c10037bf50fcff
    // | xxd -r -p > cat010ed0.31.bin

    REQUIRE(jasterix.hasCategory(10));
    std::shared_ptr<jASTERIX::Category> cat010 = jasterix.category(10);
    REQUIRE(cat010->hasEdition("0.31"));
    cat010->setCurrentEdition("0.31");
    cat010->setCurrentMapping("");

    const std::string filename = "cat010ed0.31.bin";

    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));
    REQUIRE(jASTERIX::Files::fileSize(data_path + filename) == 41);

    jasterix.decodeFile(data_path + filename, test_cat010_callback);

    loginf << "cat010 ed 0.31 test: end" << logendl;
}
