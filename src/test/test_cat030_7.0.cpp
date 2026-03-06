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

void test_cat030_callback(std::unique_ptr<nlohmann::json> json_data, size_t total_num_bytes, size_t num_frames,
                          size_t num_records, size_t num_errors)
{
    loginf << "cat030 7.0 test: decoded " << num_frames << " frames, " << num_records
           << " records, " << num_errors << " errors" << logendl;

    REQUIRE(num_frames == 0);
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

    // {
    //     "data_blocks": [
    //         {
    //             "category": 30,
    //             "content": {
    //                 "index": 3,
    //                 "length": 61,
    //                 "records": [
    //                     {
    //                         "010": {
    //                             "SAC": 0,
    //                             "SIC": 1
    //                         },
    //                         "015": {
    //                             "USER NUMBER": 36
    //                         },
    //                         "020": {
    //                             "TIME OF MESSAGE": 54780.0234375
    //                         },
    //                         "030": {
    //                             "BS": 1,
    //                             "C1": 0,
    //                             "FX": 0
    //                         },
    //                         "035": {
    //                             "FAMILY": 0,
    //                             "NATURE": 0
    //                         },
    //                         "040": {
    //                             "TRACK NUMBER": 2113,
    //                             "sttn": 1
    //                         },
    //                         "050": {
    //                             "ARTAS UNIT IDENTIFICATION": 48,
    //                             "FX": 0,
    //                             "SYSTEM TRACK NUMBER": 3047
    //                         },
    //                         "060": {
    //                             "C": 0,
    //                             "G": 0,
    //                             "Mode 3/A Code": 2274,
    //                             "V": 0
    //                         },
    //                         "070": {
    //                             "TIME OF LAST UPDATE": 54780.0234375
    //                         },
    //                         "080": {
    //                             "ADD": 0,
    //                             "AMA": 1,
    //                             "CNF": 0,
    //                             "COR": 7,
    //                             "CRE": 0,
    //                             "CST": 0,
    //                             "FOR": 0,
    //                             "FX": 1,
    //                             "FX2": 1,
    //                             "FX3": 1,
    //                             "FX4": 0,
    //                             "LIV": 0,
    //                             "ME": 0,
    //                             "SF": 1,
    //                             "SLR": 0,
    //                             "SPI": 0,
    //                             "TDC": 0,
    //                             "TRM": 0,
    //                             "TYPE": 2
    //                         },
    //                         "100": {
    //                             "X-Component": -26.734375,
    //                             "Y-Component": 58.984375
    //                         },
    //                         "130": {
    //                             "ALTITUDE": 36025.0,
    //                             "SRC": 1
    //                         },
    //                         "140": {
    //                             "GC": 0,
    //                             "MODE C": 370.0,
    //                             "VAL": 0
    //                         },
    //                         "150": {
    //                             "MODE C": 370.0
    //                         },
    //                         "170": {
    //                             "AMODE": 1.75,
    //                             "CMODE": 2.25,
    //                             "PSR": 63.75,
    //                             "SSR": 1.75
    //                         },
    //                         "180": {
    //                             "HEADING": 211.31103506008,
    //                             "SPEED": 0.12677000655
    //                         },
    //                         "200": {
    //                             "LONGI": 0,
    //                             "TRANS": 0,
    //                             "VERTI": 0
    //                         },
    //                         "220": {
    //                             "RATE OF C/D": 0.0
    //                         },
    //                         "240": {
    //                             "CALCULATED RATE OF TURN": 0.0
    //                         },
    //                         "260": {
    //                             "SAC": 0,
    //                             "SIC": 2
    //                         },
    //                         "340": {
    //                             "G": 0,
    //                             "L": 0,
    //                             "Mode 3/A Code": 2274,
    //                             "V": 0
    //                         },
    //                         "360": {
    //                             "RHO": 200.703125,
    //                             "THETA": 214.41467275398
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
    //                             false,
    //                             true,
    //                             true,
    //                             true,
    //                             false,
    //                             true,
    //                             true,
    //                             false,
    //                             true,
    //                             true,
    //                             true,
    //                             false,
    //                             true,
    //                             true,
    //                             true,
    //                             true,
    //                             true,
    //                             false,
    //                             false,
    //                             false,
    //                             false,
    //                             true,
    //                             false,
    //                             false,
    //                             false,
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
    //                             true,
    //                             false,
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
    //                             false
    //                         ],
    //                         "index": 3,
    //                         "length": 61,
    //                         "record_data": "ffddbbe10141018000010024040018416afe03ff070709f9510ebf081d964404bc05c845a1050f1180000000000002645a987905c804bc6afe030617ce"
    //                     }
    //                 ]
    //             },
    //             "length": 64
    //         }
    //     ],
    //     "rec_num": 0
    // }

    loginf << "cat030 7.0 test: data block" << logendl;

    REQUIRE(json_data->contains("data_blocks"));
    REQUIRE(json_data->at("data_blocks").is_array());
    REQUIRE(json_data->at("data_blocks").size() == 1);

    const json& first_data_block = json_data->at("data_blocks").at(0);

    REQUIRE(first_data_block.contains("category"));
    REQUIRE(first_data_block.at("category") == 30);
    REQUIRE(first_data_block.contains("length"));
    REQUIRE(first_data_block.at("length") == 64);

    loginf << "cat030 7.0 test: num records" << logendl;
    REQUIRE(first_data_block.contains("content"));
    REQUIRE(first_data_block.at("content").contains("records"));
    REQUIRE(first_data_block.at("content").at("records").is_array());
    REQUIRE(first_data_block.at("content").at("records").size() == 1);

    const json& record = first_data_block.at("content").at("records").at(0);

    //    ; FSPEC: 0x ff dd bb e1 01 41 01 80
    // 11111111 11011101 10111011 11100001 00000001 01000001 00000001 10000000

    loginf << "cat030 7.0 test: fspec" << logendl;


    // ;  I030/010: =0x 00 01
    // ;  Server Identification Tag: 0x0001 (SAC=0; SIC=1)

    loginf << "cat030 7.0 test: 010" << logendl;
    REQUIRE(record.at("010").at("SAC") == 0);
    REQUIRE(record.at("010").at("SIC") == 1);

    // ;  I030/015: =0x 00 24
    // ;  User Number: 36

    loginf << "cat030 7.0 test: 015" << logendl;
    REQUIRE(record.at("015").at("USER NUMBER") == 36);

    // ;  I030/030: =0x 04
    // ;  Service Identification: BS

    loginf << "cat030 7.0 test: 030" << logendl;
    REQUIRE(record.at("030").at("BS") == 1);
    REQUIRE(record.at("030").at("C1") == 0);

    // ;  I030/035: =0x 00
    // ;  Type of Message: family=0; nature=0

    loginf << "cat030 7.0 test: 035" << logendl;
    REQUIRE(record.at("035").at("FAMILY") == 0);
    REQUIRE(record.at("035").at("NATURE") == 0);

    // ;  I030/040: =0x 18 41
    // ;  Track Number: sttn=1; stn=2113

    loginf << "cat030 7.0 test: 040" << logendl;
    REQUIRE(record.at("040").at("sttn") == 1);
    REQUIRE(record.at("040").at("TRACK NUMBER") == 2113);

    // ;  I030/070: =0x 6a fe 03
    // ;  Time of Last Update: 0x6afe03 (7011843; 15:13:00.023 UTC); tolu=54780.023438

    loginf << "cat030 7.0 test: 070" << logendl;
    REQUIRE(approximatelyEqual(record.at("070").at("TIME OF LAST UPDATE"), 54780.0234375, 10e-7));

    // ;  I030/170: =0x ff 07 07 09 
    // ;  Track Ages: PSR=255 (63.75 sec); SSR=7 (1.75 sec)
    // ;    amode=7 (1.75 sec); cmode=9 (2.25 sec)

    loginf << "cat030 7.0 test: 170" << logendl;
    REQUIRE(record.at("170").at("AMODE") == 1.75);
    REQUIRE(record.at("170").at("CMODE") == 2.25);
    REQUIRE(record.at("170").at("PSR") == 63.75);
    REQUIRE(record.at("170").at("SSR") == 1.75);

    // ;  I030/100: =0x f9 51 0e bf 
    // ;  Calculated Track Position: x=-1711 (-26.734 or -53.469 nmi); y=3775 (58.984 or 117.969 nmi)

    loginf << "cat030 7.0 test: 100" << logendl;
    REQUIRE(approximatelyEqual(record.at("100").at("X-Component"), -26.734375, 10e-6));
    REQUIRE(approximatelyEqual(record.at("100").at("Y-Component"), 58.984375, 10e-6));

    // ;  I030/180: =0x 08 1d 96 44 
    // ;  Calculated Track Velocity: spd=2077 (456.372 kts); hdg=38468 (211.311 deg)

    loginf << "cat030 7.0 test: 180" << logendl;
    REQUIRE(approximatelyEqual(record.at("180").at("HEADING"), 211.31103506008, 10e-11));
    REQUIRE(approximatelyEqual(record.at("180").at("SPEED"), 0.12677000655, 10e-11));

    // ;  I030/060: =0x 04 bc
    // ;  Track Mode 3/A: v=0; g=0; c=0; code=02274

    loginf << "cat030 7.0 test: 060" << logendl;
    REQUIRE(record.at("060").at("C") == 0);
    REQUIRE(record.at("060").at("G") == 0);
    REQUIRE(record.at("060").at("V") == 0);
    REQUIRE(record.at("060").at("Mode 3/A Code") == 2274);

    // ;  I030/150: =0x 05 c8
    // ;  Measured Track Mode C: value=1480 (370.00 FL)

    loginf << "cat030 7.0 test: 150" << logendl;
    REQUIRE(record.at("150").at("MODE C") == 370.0);

    // ;  I030/130: =0x 45 a1
    // ;  Calculated Track Altitude: src=1 (triangulated height); value=1441 (360.25 FL)

    loginf << "cat030 7.0 test: 130" << logendl;
    REQUIRE(record.at("130").at("ALTITUDE") == 36025.0);
    REQUIRE(record.at("130").at("SRC") == 1);

    // ;  I030/080: =0x 05 0f 11 80 
    // ;  ARTAS Track Status:
    // ;   LIV CNF TYPE=2 (SSR only multiradar track)
    // ;   SLR=0 COR=7
    // ;   AMA
    // ;   SF=1 (1/32 nmi)

    loginf << "cat030 7.0 test: 080" << logendl;
    REQUIRE(record.at("080").at("ADD") == 0);
    REQUIRE(record.at("080").at("AMA") == 1);
    REQUIRE(record.at("080").at("CNF") == 0);
    REQUIRE(record.at("080").at("COR") == 7);
    REQUIRE(record.at("080").at("CRE") == 0);
    REQUIRE(record.at("080").at("CST") == 0);
    REQUIRE(record.at("080").at("FOR") == 0);
    REQUIRE(record.at("080").at("LIV") == 0);
    REQUIRE(record.at("080").at("ME") == 0);
    REQUIRE(record.at("080").at("SF") == 1);
    REQUIRE(record.at("080").at("SLR") == 0);
    REQUIRE(record.at("080").at("SPI") == 0);
    REQUIRE(record.at("080").at("TDC") == 0);
    REQUIRE(record.at("080").at("TRM") == 0);
    REQUIRE(record.at("080").at("TYPE") == 2);

    // ;  I030/200: =0x 00
    // ;  Mode of Flight: trans=0 (constant course) ; longi=0 (constant groundspeed) ; verti=0 (level flight)

    loginf << "cat030 7.0 test: 200" << logendl;
    REQUIRE(record.at("200").at("LONGI") == 0);
    REQUIRE(record.at("200").at("TRANS") == 0);
    REQUIRE(record.at("200").at("VERTI") == 0);

    // ;  I030/220: =0x 00 00
    // ;  Calculated Rate of Climb/Descent: crocd=0 (0.00 ft/min)

    loginf << "cat030 7.0 test: 220" << logendl;
    REQUIRE(record.at("220").at("RATE OF C/D") == 0.0);

    // ;  I030/240: =0x 00
    // ;  Calculated Rate of Turn: 0 (0.00 deg/sec)

    loginf << "cat030 7.0 test: 240" << logendl;
    REQUIRE(record.at("240").at("CALCULATED RATE OF TURN") == 0.0);

    // ;  I030/260: =0x 14 0a
    // ;  Radar Identification Tag: 0x0002 (SAC=0; SIC=2)

    loginf << "cat030 7.0 test: 260" << logendl;
    REQUIRE(record.at("260").at("SAC") == 0);
    REQUIRE(record.at("260").at("SIC") == 2);

    // ;  I030/360: =0x 64 5a 98 79 
    // ;  Measured Position: rho=25690 (200.703 nmi); theta=39033 (214.415 deg)

    loginf << "cat030 7.0 test: 360" << logendl;
    REQUIRE(approximatelyEqual(record.at("360").at("RHO"), 200.703125, 10e-6));
    REQUIRE(approximatelyEqual(record.at("360").at("THETA"), 214.41467275398, 10e-11));

    // ;  I030/140: =0x 05 c8
    // ;  Last Measured Mode C: v=0; g=0; value=1480 (370.00 FL)

    loginf << "cat030 7.0 test: 140" << logendl;
    REQUIRE(record.at("140").at("GC") == 0);
    REQUIRE(record.at("140").at("VAL") == 0);
    REQUIRE(record.at("140").at("MODE C") == 370.0);

    // ;  I030/340: =0x 04 bc
    // ;  Last Measured Mode 3/A: v=0; g=0; l=0; code=02274

    loginf << "cat030 7.0 test: 340" << logendl;
    REQUIRE(record.at("340").at("G") == 0);
    REQUIRE(record.at("340").at("L") == 0);
    REQUIRE(record.at("340").at("V") == 0);
    REQUIRE(record.at("340").at("Mode 3/A Code") == 2274);

    // ;  I030/020: =0x 6a fe 03
    // ;  Time of Message: 0x6afe03 (7011843; 15:13:00.023 UTC)

    loginf << "cat030 7.0 test: 020" << logendl;
    REQUIRE(approximatelyEqual(record.at("020").at("TIME OF MESSAGE"), 54780.0234375, 10e-7));

    // ;  I030/050: 0x 06 17 ce
    // ;  ARTAS Track Number: unit=48; stnr=3047

    loginf << "cat030 7.0 test: 050" << logendl;
    REQUIRE(record.at("050").at("ARTAS UNIT IDENTIFICATION") == 48);
    REQUIRE(record.at("050").at("SYSTEM TRACK NUMBER") == 3047);
}

TEST_CASE("jASTERIX CAT030 7.0", "[jASTERIX CAT030]")
{
    loginf << "cat030 7.0 test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, true, true, false);

    //    ; ASTERIX data block at pos 0: cat=30; len=64
    // 1e0040ffddbbe10141018000010024040018416afe03ff070709f9510ebf081d964404bc05c845a1050f1180000000000002645a987905c804bc6afe030617ce

    // echo -n
    // 1e0040ffddbbe10141018000010024040018416afe03ff070709f9510ebf081d964404bc05c845a1050f1180000000000002645a987905c804bc6afe030617ce
    // | xxd -r -p > cat030ed7.0.bin

    REQUIRE(jasterix.hasCategory(30));
    std::shared_ptr<jASTERIX::Category> cat030 = jasterix.category(30);
    REQUIRE(cat030->hasEdition("7.0"));
    cat030->setCurrentEdition("7.0");
    cat030->setCurrentMapping("");

    const std::string filename = "cat030ed7.0.bin";

    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));
    REQUIRE(jASTERIX::Files::fileSize(data_path + filename) == 64);

    jasterix.decodeFile(data_path + filename, test_cat030_callback);

    loginf << "cat030 7.0 test: end" << logendl;
}
