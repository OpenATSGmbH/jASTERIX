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

void test_cat063_callback(std::unique_ptr<nlohmann::json> json_data, size_t total_num_bytes, size_t num_frames,
                          size_t num_records, size_t num_errors)
{
    loginf << "cat063 test: decoded " << num_frames << " frames, " << num_records << " records, "
           << num_errors << " errors" << logendl;

    REQUIRE(num_frames == 0);
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

    //    {
    //        "data_blocks": [
    //            {
    //                "category": 63,
    //                "content": {
    //                    "index": 3,
    //                    "length": 27,
    //                    "records": [
    //                        {
    //                            "010": {
    //                                "SAC": 0,
    //                                "SIC": 5
    //                            },
    //                            "015": {
    //                                "Service Identification": 193
    //                            },
    //                            "030": {
    //                                "Time of Message": 33509.796875
    //                            },
    //                            "050": {
    //                                "SAC": 0,
    //                                "SIC": 1
    //                            },
    //                            "060": {
    //                                "ADS": 0,
    //                                "CON": 0,
    //                                "FX": 0,
    //                                "MDS": 0,
    //                                "MLT": 0,
    //                                "PSR": 0,
    //                                "SSR": 0
    //                            },
    //                            "070": {
    //                                "TSB": -53.0
    //                            },
    //                            "080": {
    //                                "SRB": 0.0,
    //                                "SRG": 0.00011
    //                            },
    //                            "081": {
    //                                "SAB": -0.00549316406
    //                            },
    //                            "090": {
    //                                "PRB": 0.0078125,
    //                                "PRG": 1e-05
    //                            },
    //                            "091": {
    //                                "PAB": -0.01098632812
    //                            },
    //                            "092": {
    //                                "PEB": 0.0
    //                            },
    //                            "FSPEC": [
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                false,
    //                                false,
    //                                false,
    //                                false
    //                            ]
    //                        }
    //                    ]
    //                },
    //                "length": 30
    //            }
    //        ]
    //    }

    loginf << "cat063 test: data block" << logendl;

    REQUIRE(json_data->contains("data_blocks"));
    REQUIRE(json_data->at("data_blocks").is_array());
    REQUIRE(json_data->at("data_blocks").size() == 1);

    const json& first_data_block = json_data->at("data_blocks").at(0);

    REQUIRE(first_data_block.contains("category"));
    REQUIRE(first_data_block.at("category") == 63);
    REQUIRE(first_data_block.contains("length"));
    REQUIRE(first_data_block.at("length") == 30);

    loginf << "cat063 test: num records" << logendl;
    REQUIRE(first_data_block.contains("content"));
    REQUIRE(first_data_block.at("content").contains("records"));
    REQUIRE(first_data_block.at("content").at("records").is_array());
    REQUIRE(first_data_block.at("content").at("records").size() == 1);

    const json& record = first_data_block.at("content").at("records").at(0);

    //    ; FSPEC: 0x ff f0

    loginf << "cat063 test: fspec" << logendl;


    //    ; Data Record:
    //    ;  I063/010: =0x 00 05
    //    ;  Data Source Identifier: 0x0005 (SAC=0; SIC=5)

    loginf << "cat063 test: 010" << logendl;
    REQUIRE(record.at("010").at("SAC") == 0);
    REQUIRE(record.at("010").at("SIC") == 5);

    //    ;  I063/015: =0x c1
    //    ;  Service Identification: 193

    loginf << "cat063 test: 015" << logendl;
    REQUIRE(record.at("015").at("Service Identification") == 193);

    //    ;  I063/030: =0x 41 72 e6
    //    ;  Time of Message: 0x4172e6 (4289254; 09:18:29.797 UTC)

    loginf << "cat063 test: 030" << logendl;
    REQUIRE(approximatelyEqual(record.at("030").at("Time of Message"), 33509.796875, 10e-6));

    //    ;  I063/050: =0x 00 01
    //    ;  Sensor Identifier: 0x0001 (SAC=0; SIC=1)

    loginf << "cat063 test: 050" << logendl;
    REQUIRE(record.at("050").at("SAC") == 0);
    REQUIRE(record.at("050").at("SIC") == 1);

    //    ;  I063/060: =0x 00
    //    ;  Sensor Configuration and Status: con=0 (operational); PSR GO; SSR GO; ModeS GO; ADS GO;
    //    MLT GO

    loginf << "cat063 test: 060" << logendl;
    REQUIRE(record.at("060").at("CON") == 0);
    REQUIRE(record.at("060").at("PSR") == 0);
    REQUIRE(record.at("060").at("SSR") == 0);
    REQUIRE(record.at("060").at("MDS") == 0);
    REQUIRE(record.at("060").at("ADS") == 0);
    REQUIRE(record.at("060").at("MLT") == 0);

    //    ;  I063/070: =0x ff cb
    //    ;  Time Stamping Bias: -53 msec

    loginf << "cat063 test: 070" << logendl;
    REQUIRE(record.at("070").at("TSB") == -53);

    //    ;  I063/080: =0x 00 0b 00 00
    //    ;  SSR/ModeS Range Gain and Bias: bias=0 (0.000 nmi) gain=11 (0.000110)

    loginf << "cat063 test: 080" << logendl;
    REQUIRE(record.at("080").at("SRG") == 0.00011);
    REQUIRE(record.at("080").at("SRB") == 0.0);

    //    ;  I063/081: =0x ff ff
    //    ;  SSR/ModeS Azimuth Bias: -1 (-0.005 deg)

    loginf << "cat063 test: 081" << logendl;
    REQUIRE(record.at("081").at("SAB") == -0.00549316406);

    //    ;  I063/090: =0x 00 01 00 01
    //    ;  PSR Range Gain and Bias: bias=1 (0.008 nmi) gain=1 (0.000010)

    loginf << "cat063 test: 090" << logendl;
    REQUIRE(record.at("090").at("PRG") == 0.00001);
    REQUIRE(record.at("090").at("PRB") == 0.0078125);

    //    ;  I063/091: =0x ff fe
    //    ;  PSR Azimuth Bias: -2 (-0.011 deg)

    loginf << "cat063 test: 091" << logendl;
    REQUIRE(record.at("091").at("PAB") == -0.01098632812);

    //    ;  I063/092: =0x 00 00
    //    ;  PSR Elevation Bias: 0 (0.000 deg)

    loginf << "cat063 test: 092" << logendl;
    REQUIRE(record.at("092").at("PEB") == 0.0);
}

TEST_CASE("jASTERIX CAT063 1.0", "[jASTERIX CAT063]")
{
    loginf << "cat063 test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, true, true, false);

    //  	  ; ASTERIX data block at pos 0: cat=63; len=30
    //      3f001efff00005c14172e6000100ffcb000b0000ffff00010001fffe0000

    // echo -n 3f001efff00005c14172e6000100ffcb000b0000ffff00010001fffe0000 | xxd -r -p >
    // cat063ed1.0.bin

    // REQUIRE (size == 30);

    REQUIRE(jasterix.hasCategory(63));
    std::shared_ptr<jASTERIX::Category> cat063 = jasterix.category(63);
    REQUIRE(cat063->hasEdition("1.0"));
    cat063->setCurrentEdition("1.0");
    cat063->setCurrentMapping("");

    const std::string filename = "cat063ed1.0.bin";

    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));
    REQUIRE(jASTERIX::Files::fileSize(data_path + filename) == 30);

    jasterix.decodeFile(data_path + filename, test_cat063_callback);

    loginf << "cat063 test: end" << logendl;
}
