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

void test_cat048_123_callback(std::unique_ptr<nlohmann::json> json_data, size_t total_num_bytes, size_t num_frames,
                          size_t num_records, size_t num_errors)
{
    loginf << "cat048 test: decoded " << num_frames << " frames, " << num_records << " records, "
           << num_errors << " errors" << logendl;

    REQUIRE(num_frames == 0);
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

    // {
    //     "data_blocks": [
    //         {
    //             "category": 48,
    //             "content": {
    //                 "index": 3,
    //                 "length": 69,
    //                 "records": [
    //                     {
    //                         "010": {
    //                             "SAC": 0,
    //                             "SIC": 1
    //                         },
    //                         "020": {
    //                             "ERR": 1,
    //                             "FOE/FRI": 0,
    //                             "FX": 1,
    //                             "FX2": 0,
    //                             "ME": 0,
    //                             "MI": 1,
    //                             "RAB": 0,
    //                             "RDP": 1,
    //                             "SIM": 0,
    //                             "SPI": 0,
    //                             "TST": 0,
    //                             "TYP": 5,
    //                             "XPP": 0
    //                         },
    //                         "040": {
    //                             "RHO": 255.99609375,
    //                             "THETA": 89.67041011543999
    //                         },
    //                         "070": {
    //                             "G": 0,
    //                             "L": 1,
    //                             "Mode-3/A reply": 470,
    //                             "V": 0
    //                         },
    //                         "090": {
    //                             "Flight Level": 370.0,
    //                             "G": 0,
    //                             "V": 0
    //                         },
    //                         "130": {
    //                             "SAM": {
    //                                 "value": -63
    //                             },
    //                             "available": [
    //                                 false,
    //                                 false,
    //                                 true,
    //                                 false,
    //                                 false,
    //                                 false,
    //                                 false,
    //                                 false
    //                             ]
    //                         },
    //                         "140": {
    //                             "Time-of-Day": 33499.8359375
    //                         },
    //                         "161": {
    //                             "TRACK NUMBER": 919
    //                         },
    //                         "170": {
    //                             "CDM": 0,
    //                             "CNF": 0,
    //                             "DOU": 0,
    //                             "FX": 0,
    //                             "MAH": 0,
    //                             "RAD": 2
    //                         },
    //                         "200": {
    //                             "CALCULATED GROUNDSPEED": 463.1835467201585,
    //                             "CALCULATED HEADING": 32.60742186016
    //                         },
    //                         "220": {
    //                             "AIRCRAFT ADDRESS": 11226301
    //                         },
    //                         "230": {
    //                             "AIC": 1,
    //                             "ARC": 1,
    //                             "B1A": 1,
    //                             "B1B": 13,
    //                             "COM": 1,
    //                             "MSSC": 1,
    //                             "SI": 0,
    //                             "STAT": 0
    //                         },
    //                         "240": {
    //                             "Aircraft Identification": "RYR5XW  "
    //                         },
    //                         "250": {
    //                             "Mode S MB Data": [
    //                                 {
    //                                     "BDS1": 6,
    //                                     "BDS2": 0,
    //                                     "MB Data": "8bd9eb2fbfe400"
    //                                 },
    //                                 {
    //                                     "BDS1": 5,
    //                                     "BDS2": 0,
    //                                     "MB Data": "80919f39a004dd"
    //                                 },
    //                                 {
    //                                     "BDS1": 4,
    //                                     "BDS2": 0,
    //                                     "MB Data": "c8480030a80000"
    //                                 }
    //                             ],
    //                             "REP": 3
    //                         },
    //                         "FSPEC": [
    //                             true,
    //                             true,
    //                             true,
    //                             true,
    //                             true,
    //                             true,
    //                             true,
    //                             true,
    //                             true,
    //                             true,
    //                             true,
    //                             true,
    //                             false,
    //                             true,
    //                             true,
    //                             true,
    //                             false,
    //                             false,
    //                             false,
    //                             false,
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
    //                             true,
    //                             false
    //                         ],
    //                         "REF": {
    //                             "ERR": {
    //                                 "RHO": 257.0
    //                             },
    //                             "REF_FSPEC": [
    //                                 false,
    //                                 false,
    //                                 false,
    //                                 false,
    //                                 true,
    //                                 false,
    //                                 false,
    //                                 false
    //                             ]
    //                         },
    //                         "index": 3,
    //                         "length": 69
    //                     }
    //                 ]
    //             },
    //             "length": 72
    //         }
    //     ],
    //     "rec_num": 0
    // }

    loginf << "cat048 test: data block" << logendl;

    REQUIRE(json_data->contains("data_blocks"));
    REQUIRE(json_data->at("data_blocks").is_array());
    REQUIRE(json_data->at("data_blocks").size() == 1);

    const json& first_data_block = json_data->at("data_blocks").at(0);

    REQUIRE(first_data_block.contains("category"));
    REQUIRE(first_data_block.at("category") == 48);
    REQUIRE(first_data_block.contains("length"));
    REQUIRE(first_data_block.at("length") == 72);

    loginf << "cat048 test: num records" << logendl;
    REQUIRE(first_data_block.contains("content"));
    REQUIRE(first_data_block.at("content").contains("records"));
    REQUIRE(first_data_block.at("content").at("records").is_array());
    REQUIRE(first_data_block.at("content").at("records").size() == 1);

    const json& record = first_data_block.at("content").at("records").at(0);

    //    ; FSPEC: 0x ff f7 03 02
    // 11111111111101110000001100000010

    loginf << "cat048 test: fspec" << logendl;


    //    ; Data Record:
    //    ;  I048/010: =0x 00 01
    //    ;  Data Source Identifier: 0x0001 (SAC=0; SIC=1)

    loginf << "cat048 test: 010" << logendl;
    REQUIRE(record.at("010").at("SAC") == 0);
    REQUIRE(record.at("010").at("SIC") == 1);

    //    ;  I048/140: =0x 41 6d eb
    //    ;  Time of Day: 0x416deb (4287979; 33499.835938 secs; 09:18:19.836 UTC)

    loginf << "cat048 test: 140" << logendl;
    double tmp_d = record.at("140").at("Time-of-Day");
    REQUIRE(fabs(tmp_d - 33499.835938) < 10e-6);

    //    ;  I048/020: =0x a948
    //    ;  Target Report Descriptor: TYP='Single ModeS Roll-Call' ACT RDP-2 ERR MI

    loginf << "cat048 test: 020" << logendl;
    REQUIRE(record.at("020").at("TYP") == 5);
    REQUIRE(record.at("020").at("SIM") == 0);
    REQUIRE(record.at("020").at("RDP") == 1);
    REQUIRE(record.at("020").at("SPI") == 0);
    REQUIRE(record.at("020").at("RAB") == 0);
    REQUIRE(record.at("020").at("ERR") == 1);
    REQUIRE(record.at("020").at("MI") == 1);

    //    ;  I048/040: =0x ff ff 3f c4 
    //    ;  Measured Position: srg=65535 (255.996 nmi); azm=16324 (89.670 deg)

    loginf << "cat048 test: 040" << logendl;
    REQUIRE(approximatelyEqual(record.at("040").at("RHO"), 255.99609375, 10e-8));
    REQUIRE(approximatelyEqual(record.at("040").at("THETA"), 89.67041015625, 10e-10));

    //    ;  I048/070: =0x 21 38
    //    ;  Mode 3/A Code: v=0; g=0; l=1; code=00470

    loginf << "cat048 test: 070" << logendl;
    REQUIRE(record.at("070").at("V") == 0);
    REQUIRE(record.at("070").at("G") == 0);
    REQUIRE(record.at("070").at("L") == 1);
    REQUIRE(record.at("070").at("Mode-3/A reply") == 470);

    //    ;  I048/090: =0x 05 c8
    //    ;  Flight Level: v=0; g=0; code=1480 (370.00 FL)

    loginf << "cat048 test: 090" << logendl;
    REQUIRE(record.at("090").at("V") == 0);
    REQUIRE(record.at("090").at("G") == 0);
    REQUIRE(record.at("090").at("Flight Level") == 370.0);

    //    ;  I048/130: 0x 20 c1
    //    ;  Radar Plot Characteristics:
    //    ;   Amplitude of (M)SSR reply: -63 dBm

    loginf << "cat048 test: 130" << logendl;

    // 00100000
    REQUIRE(record.at("130").at("SAM").at("value") == -63);

    //    ;  I048/220: =0x ab 4c bd
    //    ;  Aircraft Address: 0xab4cbd (11226301)

    loginf << "cat048 test: 220" << logendl;
    REQUIRE(record.at("220").at("AIRCRAFT ADDRESS") == 11226301);

    //    ;  I048/240: =0x 49 94 b5 61  78 20
    //    ;  Aircraft Identification: "RYR5XW  "

    loginf << "cat048 test: 240" << logendl;
    REQUIRE(record.at("240").at("Aircraft Identification") == "RYR5XW  ");

    //    ;  I048/250: =0x 03 8b d9 eb  2f bf e4 00  60 80 91 9f  39 a0 04 dd
    //    ;            +0x 50 c8 48 00  30 a8 00 00  40
    //    ;  Mode S MB Data:
    //    ;   BDS 6,0 data=0x 8b d9 eb 2f bf e4 00
    //    ;   BDS 5,0 data=0x 80 91 9f 39 a0 04 dd
    //    ;   BDS 4,0 data=0x c8 48 00 30 a8 00 00
    //    ;           mcp_fcu_selected_altitude=37008[ft]

    loginf << "cat048 test: 250" << logendl;
    REQUIRE(record.at("250").at("Mode S MB Data").size() == 3);
    REQUIRE(record.at("250").at("Mode S MB Data")[0].at("BDS1") == 6);
    REQUIRE(record.at("250").at("Mode S MB Data")[0].at("BDS2") == 0);
    REQUIRE(record.at("250").at("Mode S MB Data")[0].at("MB Data") == "8bd9eb2fbfe400");

    //    ;  I048/161: =0x 03 97
    //    ;  Track Number: num=919

    loginf << "cat048 test: 161" << logendl;
    REQUIRE(record.at("161").at("TRACK NUMBER") == 919);

    //    ;  I048/200: =0x 08 3c 17 30
    //    ;  Calculated Track Velocity: spd=2108 (463.184 kts); hdg=5936 (32.607 deg)

    loginf << "cat048 test: 200" << logendl;
    REQUIRE(approximatelyEqual(record.at("200").at("CALCULATED GROUNDSPEED"), 463.184, 10e-3));
    REQUIRE(approximatelyEqual(record.at("200").at("CALCULATED HEADING"), 32.607, 10e-3));

    //    ;  I048/170: =0x 40
    //    ;  Track Status: CNF SRT LVL

    loginf << "cat048 test: 170" << logendl;
    REQUIRE(record.at("170").at("CNF") == 0);
    REQUIRE(record.at("170").at("RAD") == 2);
    REQUIRE(record.at("170").at("DOU") == 0);
    REQUIRE(record.at("170").at("MAH") == 0);
    REQUIRE(record.at("170").at("CDM") == 0);

    //    ;  I048/230: =0x 20 fd
    //    ;  Communications Capability: CC=1; FS=0 (airborne); MSSC; ARC=25ft; AIC; B1A=1; B1B=13

    loginf << "cat048 test: 230" << logendl;
    REQUIRE(record.at("230").at("COM") == 1);
    REQUIRE(record.at("230").at("STAT") == 0);
    REQUIRE(record.at("230").at("MSSC") == 1);
    REQUIRE(record.at("230").at("ARC") == 1);
    REQUIRE(record.at("230").at("AIC") == 1);
    REQUIRE(record.at("230").at("B1A") == 1);
    REQUIRE(record.at("230").at("B1B") == 13);

    //    ;  I048/REF: 0x 05 08 01 01  00

    loginf << "cat048 test: REF: ERR" << logendl;

    REQUIRE(record.at("REF").contains("ERR"));

    const json& ref_err = record.at("REF").at("ERR");

    //    ;  Extended Range:
    //    ;   RHO = 257.0 nmi
    REQUIRE(ref_err.at("RHO") == 257.0);
}

TEST_CASE("jASTERIX CAT048 1.23", "[jASTERIX CAT048]")
{
    loginf << "cat048 test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, true, true, false);

    // echo -n 300048fff703020001416deba948ffff3fc4213805c820c1ab4cbd4994b5617820038bd9eb2fbfe400608091
    // 9f39a004dd50c8480030a80000400397083c17304020fd0508010100 | xxd -r -p > cat048ed1.23.bin

    REQUIRE(jasterix.hasCategory(48));
    std::shared_ptr<jASTERIX::Category> cat048 = jasterix.category(48);
    REQUIRE(cat048->hasEdition("1.23"));
    cat048->setCurrentEdition("1.23");
    cat048->setCurrentMapping("");

    const std::string filename = "cat048ed1.23.bin";

    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));
    REQUIRE(jASTERIX::Files::fileSize(data_path + filename) == 72);

    jasterix.decodeFile(data_path + filename, test_cat048_123_callback);

    loginf << "cat048 test: end" << logendl;
}
