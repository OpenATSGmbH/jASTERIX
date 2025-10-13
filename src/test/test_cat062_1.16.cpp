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

void test_cat062_116_callback(std::unique_ptr<nlohmann::json> json_data, size_t num_frames,
                              size_t num_records, size_t num_errors)
{
    loginf << "cat062 1.16 test: decoded " << num_frames << " frames, " << num_records
           << " records, " << num_errors << " errors" << logendl;

    REQUIRE(num_frames == 0);
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

    //    {
    //        "data_blocks": [
    //            {
    //                "category": 62,
    //                "content": {
    //                    "index": 3,
    //                    "length": 148,
    //                    "records": [
    //                        {
    //                            "010": {
    //                                "SAC": 0,
    //                                "SIC": 4
    //                            },
    //                            "015": {
    //                                "Service Identification": 225
    //                            },
    //                            "040": {
    //                                "Track Number": 5086
    //                            },
    //                            "060": {
    //                                "CH": 0,
    //                                "Mode-3/A reply": 7621
    //                            },
    //                            "070": {
    //                                "Time Of Track Information": 33502.5
    //                            },
    //                            "080": {
    //                                "AAC": 0,
    //                                "ADS": 1,
    //                                "AFF": 0,
    //                                "AMA": 0,
    //                                "CNF": 0,
    //                                "CST": 0,
    //                                "FPC": 1,
    //                                "FX": 1,
    //                                "FX2": 1,
    //                                "FX3": 1,
    //                                "KOS": 1,
    //                                "MD4": 0,
    //                                "MD5": 0,
    //                                "MDS": 0,
    //                                "ME": 0,
    //                                "MI": 0,
    //                                "MON": 0,
    //                                "MRH": 0,
    //                                "PSR": 1,
    //                                "SIM": 0,
    //                                "SPI": 0,
    //                                "SRC": 3,
    //                                "SSR": 0,
    //                                "STP": 0,
    //                                "SUC": 0,
    //                                "TSB": 0,
    //                                "TSE": 0
    //                            },
    //                            "100": {
    //                                "X": 260661.0,
    //                                "Y": -220711.5
    //                            },
    //                            "105": {
    //                                "Latitude": 45.465155734119996,
    //                                "Longitude": 17.33247308292
    //                            },
    //                            "135": {
    //                                "Calculated Track Barometric Altitude": 34975.0,
    //                                "QNH": 0
    //                            },
    //                            "136": {
    //                                "Measured Flight Level": 349.75
    //                            },
    //                            "185": {
    //                                "Vx": 207.5,
    //                                "Vy": -120.5
    //                            },
    //                            "200": {
    //                                "ADF": 0,
    //                                "LONG": 0,
    //                                "TRANS": 0,
    //                                "VERT": 0
    //                            },
    //                            "210": {
    //                                "Ax": 0.0,
    //                                "Ay": 0.0
    //                            },
    //                            "220": {
    //                                "Rate of Climb/Descent": 0.0
    //                            },
    //                            "290": {
    //                                "ES": {
    //                                    "Age": 63.75
    //                                },
    //                                "MDS": {
    //                                    "Age": 2.0
    //                                },
    //                                "MLT": {
    //                                    "Age": 63.75
    //                                },
    //                                "PSR": {
    //                                    "Age": 63.75
    //                                },
    //                                "SSR": {
    //                                    "Age": 2.0
    //                                },
    //                                "available": [
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    false,
    //                                    true,
    //                                    false,
    //                                    true,
    //                                    true,
    //                                    true,
    //                                    false
    //                                ]
    //                            },
    //                            "295": {
    //                                "BVR": {
    //                                    "Age": 2.0
    //                                },
    //                                "FSS": {
    //                                    "Age": 2.0
    //                                },
    //                                "IAR": {
    //                                    "Age": 2.0
    //                                },
    //                                "MAC": {
    //                                    "Age": 2.0
    //                                },
    //                                "MDA": {
    //                                    "Age": 2.0
    //                                },
    //                                "MFL": {
    //                                    "Age": 2.0
    //                                },
    //                                "MHG": {
    //                                    "Age": 2.0
    //                                },
    //                                "available": [
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    true,
    //                                    true,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    false,
    //                                    true,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    true,
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    false,
    //                                    false,
    //                                    true
    //                                ]
    //                            },
    //                            "340": {
    //                                "MDA": {
    //                                    "G": 0,
    //                                    "L": 1,
    //                                    "Mode-3/A reply": 7621,
    //                                    "V": 0
    //                                },
    //                                "MDC": {
    //                                    "G": 0,
    //                                    "Last Measured Mode C Code": 349.75,
    //                                    "V": 0
    //                                },
    //                                "POS": {
    //                                    "RHO": 126.45703125,
    //                                    "THETA": 128.00720209018
    //                                },
    //                                "SID": {
    //                                    "SAC": 0,
    //                                    "SIC": 1
    //                                },
    //                                "TYP": {
    //                                    "RAB": 0,
    //                                    "SIM": 0,
    //                                    "TST": 0,
    //                                    "TYP": 5
    //                                },
    //                                "available": [
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    true,
    //                                    true,
    //                                    false,
    //                                    true,
    //                                    true
    //                                ]
    //                            },
    //                            "380": {
    //                                "ADR": {
    //                                    "Target Address": 6700198
    //                                },
    //                                "BVR": {
    //                                    "Barometric Vertical Rate": -31.25
    //                                },
    //                                "FSS": {
    //                                    "AH": 0,
    //                                    "AM": 0,
    //                                    "Altitude": 35000.0,
    //                                    "MV": 0
    //                                },
    //                                "IAR": {
    //                                    "Indicated Air Speed": 266.0
    //                                },
    //                                "ID": {
    //                                    "Target Identification": "DLH9CK  "
    //                                },
    //                                "MAC": {
    //                                    "Mach Number": 0.784
    //                                },
    //                                "MHG": {
    //                                    "Magnetic Heading": 119.88281244544
    //                                },
    //                                "available": [
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    true,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    false,
    //                                    true,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    true,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    true,
    //                                    true
    //                                ]
    //                            },
    //                            "390": {
    //                                "CFL": {
    //                                    "CFL": 350.0
    //                                },
    //                                "CSN": {
    //                                    "Callsign": "DLH9CK "
    //                                },
    //                                "DEP": {
    //                                    "Departure Airport": "EDDF"
    //                                },
    //                                "DST": {
    //                                    "Destination Airport": "LBSF"
    //                                },
    //                                "FCT": {
    //                                    "FR1/FR2": 0,
    //                                    "GAT/OAT": 1,
    //                                    "HPR": 0,
    //                                    "RVSM": 1
    //                                },
    //                                "IFI": {
    //                                    "NBR": 63256965,
    //                                    "TYP": 1
    //                                },
    //                                "TAC": {
    //                                    "Type of Aircraft": "A320"
    //                                },
    //                                "TAG": {
    //                                    "SAC": 0,
    //                                    "SIC": 0
    //                                },
    //                                "WTC": {
    //                                    "Wake Turbulence Category": "M"
    //                                },
    //                                "available": [
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    false,
    //                                    true,
    //                                    true,
    //                                    true,
    //                                    true,
    //                                    true,
    //                                    true,
    //                                    true,
    //                                    true,
    //                                    true
    //                                ]
    //                            },
    //                            "500": {
    //                                "AA": {
    //                                    "AA (X-Component)": 0.5,
    //                                    "AA (Y-Component)": 0.75
    //                                },
    //                                "ABA": {
    //                                    "ABA": 0.25
    //                                },
    //                                "AGA": {
    //                                    "AGA": 1593.75
    //                                },
    //                                "APC": {
    //                                    "APC (X-Component)": 40.0,
    //                                    "APC (Y-Component)": 63.0
    //                                },
    //                                "APW": {
    //                                    "APW (Latitude Component)": 0.00056326305,
    //                                    "APW (Longitude Component)": 0.00050961895
    //                                },
    //                                "ARC": {
    //                                    "ARC": 100.0
    //                                },
    //                                "ATV": {
    //                                    "ATV (X-Component)": 2.75,
    //                                    "ATV (Y-Component)": 4.75
    //                                },
    //                                "available": [
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    false,
    //                                    true,
    //                                    true,
    //                                    true,
    //                                    true,
    //                                    true,
    //                                    true,
    //                                    true,
    //                                    false,
    //                                    true
    //                                ]
    //                            },
    //                            "FSPEC": [
    //                                true,
    //                                false,
    //                                true,
    //                                true,
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
    //                                false,
    //                                true,
    //                                true,
    //                                false
    //                            ]
    //                        }
    //                    ]
    //                },
    //                "length": 151
    //            }
    //        ]
    //    }

    loginf << "cat062 1.16 test: data block" << logendl;

    REQUIRE(json_data->contains("data_blocks"));
    REQUIRE(json_data->at("data_blocks").is_array());
    REQUIRE(json_data->at("data_blocks").size() == 1);

    const json& first_data_block = json_data->at("data_blocks").at(0);

    REQUIRE(first_data_block.contains("category"));
    REQUIRE(first_data_block.at("category") == 62);
    REQUIRE(first_data_block.contains("length"));
    REQUIRE(first_data_block.at("length") == 64);

    loginf << "cat062 1.16 test: num records" << logendl;
    REQUIRE(first_data_block.contains("content"));
    REQUIRE(first_data_block.at("content").contains("records"));
    REQUIRE(first_data_block.at("content").at("records").is_array());
    REQUIRE(first_data_block.at("content").at("records").size() == 1);

    const json& record = first_data_block.at("content").at("records").at(0);

    //    ; FSPEC: 0x bf cf 3d 0b 00

    loginf << "cat062 1.16 test: fspec" << logendl;
    REQUIRE(record.at("FSPEC").size() == 5 * 8);

    REQUIRE(record.at("FSPEC") ==
            std::vector<bool>({1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1,
                               1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0}));

    //    ; Data Record:
    //    ;  I062/010: =0x 00 05
    //    ;  Data Source Identifier: 0x0005 (SAC=0; SIC=5)

    loginf << "cat062 1.16 test: 010" << logendl;
    REQUIRE(record.at("010").at("SAC") == 0);
    REQUIRE(record.at("010").at("SIC") == 5);

    //    ;  I062/015: =0x 15
    //    ;  Service Identification: 21

    loginf << "cat062 1.16 test: 015" << logendl;
    REQUIRE(record.at("015").at("Service Identification") == 21);

    //    ;  I062/070: =0x 70 88 9d
    //    ;  Time of Track Information: 0x70889d (7375005; 16:00:17.227 UTC)

    loginf << "cat062 1.16 test: 070" << logendl;
    REQUIRE(
        approximatelyEqual(record.at("070").at("Time Of Track Information"), 57617.2265625, 10e-6));

    //    ;  I062/105: =0x 00 63 f3 2e  ff dd 64 f6
    //    ;  Calculated Position in WGS-84 Co-ordinates:
    //    ;   lat=6550318 (35:08:19.118N); lon=-2267914 (012:09:57.740W)

    loginf << "cat062 1.16 test: 105" << logendl;
    REQUIRE(approximatelyEqual(record.at("105").at("Latitude"), 35.13864398002624511719, 10e-10));
    REQUIRE(approximatelyEqual(record.at("105").at("Longitude"), -12.16603875160217285156, 10e-10));

    //    ;  I062/100: =0x f7 93 02 f3  da 58
    //    ;  Calculated Track Position (Cartesian): x=-552190 (-149.079 nmi); y=-796072 (-214.922
    //    nmi)

    loginf << "cat062 1.16 test: 100" << logendl;
    REQUIRE(record.at("100").at("X") == -276095);
    REQUIRE(record.at("100").at("Y") == -398036);

    //    ;  I062/185: =0x 01 fd 02 d5
    //    ;  Calculated Track Velocity (Cartesian): vx=509 (247.354 kts); vy=725 (352.322 kts)

    loginf << "cat062 1.16 test: 185" << logendl;
    REQUIRE(record.at("185").at("Vx") == 127.25);
    REQUIRE(record.at("185").at("Vy") == 181.25);

    //    ;  I062/210: =0x 00 00
    //    ;  Calculated Acceleration (Cartesian): ax=0.00 m/s**2; ay=0.00 m/s**2

    loginf << "cat062 1.16 test: 210" << logendl;
    REQUIRE(record.at("210").at("Ax") == 0);
    REQUIRE(record.at("210").at("Ay") == 0);

    //    ;  I062/060: =0x 0c 84
    //    ;  Track Mode 3/A Code: i=0; g=0; c=0; code=06204

    loginf << "cat062 1.16 test: 060" << logendl;
    REQUIRE(record.at("060").at("Mode-3/A reply") == 6204);
    REQUIRE(record.at("060").at("CH") == 0);

    //    ;  I062/040: =0x 15 9d
    //    ;  Track Number: 5533

    loginf << "cat062 1.16 test: 040" << logendl;
    REQUIRE(record.at("040").at("Track Number") == 5533);

    //    ;  I062/080: =0x 19 03 01 58
    //    ;  Track Status:
    //    ;   MUL MRH=0 SRC=6 (default height) CNF
    //    ;   KOS=1
    //    ;   SSR

    loginf << "cat062 1.16 test: 080" << logendl;
    // 00011001
    REQUIRE(record.at("080").at("MON") == 0);
    REQUIRE(record.at("080").at("SPI") == 0);
    REQUIRE(record.at("080").at("MRH") == 0);
    REQUIRE(record.at("080").at("SRC") == 6);
    REQUIRE(record.at("080").at("CNF") == 0);
    REQUIRE(record.at("080").at("FX") == 1);

    // 00000011
    REQUIRE(record.at("080").at("SIM") == 0);
    REQUIRE(record.at("080").at("TSE") == 0);
    REQUIRE(record.at("080").at("TSB") == 0);
    REQUIRE(record.at("080").at("FPC") == 0);
    REQUIRE(record.at("080").at("AFF") == 0);
    REQUIRE(record.at("080").at("STP") == 0);
    REQUIRE(record.at("080").at("KOS") == 1);
    REQUIRE(record.at("080").at("FX2") == 1);

    // 00000001
    REQUIRE(record.at("080").at("AMA") == 0);
    REQUIRE(record.at("080").at("MD4") == 0);
    REQUIRE(record.at("080").at("ME") == 0);
    REQUIRE(record.at("080").at("MI") == 0);
    REQUIRE(record.at("080").at("MD5") == 0);
    REQUIRE(record.at("080").at("FX3") == 1);

    // 01011000
    // PSR, SSR, MDS, ADS listed if 0
    REQUIRE(record.at("080").at("CST") == 0);
    REQUIRE(record.at("080").at("PSR") == 1);
    REQUIRE(record.at("080").at("SSR") == 0);  // SSR
    REQUIRE(record.at("080").at("MDS") == 1);
    REQUIRE(record.at("080").at("ADS") == 1);
    REQUIRE(record.at("080").at("SUC") == 0);
    REQUIRE(record.at("080").at("AAC") == 0);
    REQUIRE(record.at("080").at("FX4") == 0);

    //    ;  I062/290: 0x 70 ff 24 ff
    //    ;  System Track Update Ages:
    //    ;   PSR age: 255 (63.75 sec)
    //    ;   SSR age: 36 (9.00 sec)
    //    ;   MDS age: 255 (63.75 sec)

    loginf << "cat062 1.16 test: 290" << logendl;

    // 01110000
    REQUIRE(record.at("290").at("available") == std::vector<bool>({0, 1, 1, 1, 0, 0, 0, 0}));

    REQUIRE(record.at("290").at("PSR").at("Age") == 63.75);
    REQUIRE(record.at("290").at("SSR").at("Age") == 9.00);
    REQUIRE(record.at("290").at("MDS").at("Age") == 63.75);

    //    ;  I062/136: =0x 05 f0
    //    ;  Measured Flight Level: 1520 (380.00 FL)

    loginf << "cat062 1.16 test: 136" << logendl;
    REQUIRE(record.at("136").at("Measured Flight Level") == 380.0);

    //    ;  I062/130: =0x 15 c6
    //    ;  Calculated Track Geometric Altitude: 5574 (348.38 FL)

    loginf << "cat062 1.16 test: 130" << logendl;
    REQUIRE(record.at("130").at("Altitude") == 34837.5);

    //    ;  I062/135: =0x 05 f0
    //    ;  Calculated Track Barometric Altitude: qnh=0; alt=1520 (380.00 FL)

    loginf << "cat062 1.16 test: 135" << logendl;
    REQUIRE(record.at("135").at("QNH") == 0.0);
    REQUIRE(record.at("135").at("Calculated Track Barometric Altitude") == 38000);

    //    ;  I062/220: =0x 00 00
    //    ;  Calculated Rate of Climb/Descent: 0 (0.00 ft/min)

    loginf << "cat062 1.16 test: 220" << logendl;
    REQUIRE(record.at("220").at("Rate of Climb/Descent") == 0.0);

    //    ;  I062/510: 0x 06 1b be
    //    ;  Composed Track Number:
    //    ;   Master track number: sui=6; stn=3551

    loginf << "cat062 1.16 test: 510" << logendl;
    REQUIRE(record.at("510").at("Composed Track Number")[0].at("SYSTEM UNIT IDENTIFICATION") == 6);
    REQUIRE(record.at("510").at("Composed Track Number")[0].at("SYSTEM TRACK NUMBER") == 3551);
    REQUIRE(record.at("510").at("Composed Track Number")[0].at("extend") == 0);

    loginf << "cat062 1.16 test: 340" << logendl;

    //    ;  I062/340: 0x 98 00 03 05  f0 0c 84
    // 1001 1000 0000 0000
    REQUIRE(record.at("340").at("available").size() == 8);

    REQUIRE(record.at("340").at("available") == std::vector<bool>({1, 0, 0, 1, 1, 0, 0, 0}));

    //    ;  Measured Information:
    //    ;   Sensor identification: SAC=0; SIC=3
    REQUIRE(record.at("340").at("SID").at("SAC") == 0);
    REQUIRE(record.at("340").at("SID").at("SIC") == 3);
    //    ;   Last measured mode C code: v=0; g=0; code=1520 (380.00 FL)
    REQUIRE(record.at("340").at("MDC").at("V") == 0);
    REQUIRE(record.at("340").at("MDC").at("G") == 0);
    REQUIRE(record.at("340").at("MDC").at("Last Measured Mode C Code") == 380.0);
    //    ;   Last measured mode 3/A code: v=0; g=0; l=0; code=06204
    REQUIRE(record.at("340").at("MDA").at("V") == 0);
    REQUIRE(record.at("340").at("MDA").at("G") == 0);
    REQUIRE(record.at("340").at("MDA").at("L") == 0);
    REQUIRE(record.at("340").at("MDA").at("Mode-3/A reply") == 6204);

    //     [--:--:--.---] U 16:00:17.227 -- 0x0005 A- T:US+ #5533 UV -149.079 -214.922 A:06204 ---
    //     C: 380    -- ; G:348.38
}

TEST_CASE("jASTERIX CAT062 1.16", "[jASTERIX CAT062]")
{
    loginf << "cat062 1.16 test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, true, true, false);

    //    ; ASTERIX data block at pos 0: cat=62; len=64
    //  3e0040bfcf3d0b0000051570889d0063f32effdd64f6f79302f3da5801fd02d500000c84159d1903015870ff24ff05f015c605f00000061bbe98000305f00c84

    // echo -n
    // 3e0040bfcf3d0b0000051570889d0063f32effdd64f6f79302f3da5801fd02d500000c84159d1903015870ff24ff05f015c605f00000061bbe98000305f00c84
    // | xxd -r -p > cat062ed1.16.bin

    REQUIRE(jasterix.hasCategory(62));
    std::shared_ptr<jASTERIX::Category> cat062 = jasterix.category(62);
    REQUIRE(cat062->hasEdition("1.16"));
    cat062->setCurrentEdition("1.16");
    cat062->setCurrentMapping("");

    const std::string filename = "cat062ed1.16.bin";

    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));
    REQUIRE(jASTERIX::Files::fileSize(data_path + filename) == 64);

    jasterix.decodeFile(data_path + filename, test_cat062_116_callback);

    loginf << "cat062 1.16 test: end" << logendl;
}
