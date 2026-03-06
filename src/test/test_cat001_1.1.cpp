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

void test_cat001_callback(std::unique_ptr<nlohmann::json> json_data, size_t total_num_bytes, size_t num_frames,
                          size_t num_records, size_t num_errors)
{
    loginf << "cat001 test: callback" << logendl;

    loginf << "cat001 test: decoded " << num_frames << " frames, " << num_records << " records, "
           << num_errors << " errors" << logendl;

    REQUIRE(num_frames == 0);
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

    //    {
    //        "data_blocks": [
    //            {
    //                "category": 1,
    //                "content": {
    //                    "index": 3,
    //                    "length": 17,
    //                    "records": [
    //                        {
    //                            "010": {
    //                                "SAC": 0,
    //                                "SIC": 1
    //                            },
    //                            "020": {
    //                                "ANT": 0,
    //                                "FX": 0,
    //                                "RAB": 0,
    //                                "SIM": 0,
    //                                "SPI": 0,
    //                                "SSR/PSR": 2,
    //                                "TYP": 0
    //                            },
    //                            "040": {
    //                                "RHO": 127.4375,
    //                                "THETA": 256.61865234375
    //                            },
    //                            "070": {
    //                                "G": 0,
    //                                "L": 0,
    //                                "Mode-3/A reply": 5543,
    //                                "V": 0
    //                            },
    //                            "090": {
    //                                "G": 0,
    //                                "Mode-C HEIGHT": 38000.0,
    //                                "V": 0
    //                            },
    //                            "130": {
    //                                "Radar Plot Characteristics": [
    //                                    {
    //                                        "Value": 96,
    //                                        "extend": 1
    //                                    },
    //                                    {
    //                                        "Value": 60,
    //                                        "extend": 1
    //                                    },
    //                                    {
    //                                        "Value": 96,
    //                                        "extend": 0
    //                                    }
    //                                ]
    //                            },
    //                            "141": {
    //                                "Truncated Time of Day": 221.4296875
    //                            },
    //                            "FSPEC": [
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                true,
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

    loginf << "cat001 test: data block" << logendl;

    REQUIRE(json_data->contains("data_blocks"));
    REQUIRE(json_data->at("data_blocks").is_array());
    REQUIRE(json_data->at("data_blocks").size() == 1);

    const json& first_data_block = json_data->at("data_blocks").at(0);

    REQUIRE(first_data_block.contains("category"));
    REQUIRE(first_data_block.at("category") == 1);
    REQUIRE(first_data_block.contains("length"));
    REQUIRE(first_data_block.at("length") == 20);

    loginf << "cat001 test: num records" << logendl;
    REQUIRE(first_data_block.contains("content"));
    REQUIRE(first_data_block.at("content").contains("records"));
    REQUIRE(first_data_block.at("content").at("records").is_array());
    REQUIRE(first_data_block.at("content").at("records").size() == 1);

    const json& record = first_data_block.at("content").at("records").at(0);

    //    ; FSPEC: 0x fe
    // 11111110

    loginf << "cat001 test: fspec" << logendl;


    //    ; Data Record:
    //    ;  I001/010: =0x 00 01
    //    ;  Data Source Identifier: 0x0001 (SAC=0; SIC=1)

    loginf << "cat001 test: 010" << logendl;
    REQUIRE(record.at("010").at("SAC") == 0);
    REQUIRE(record.at("010").at("SIC") == 1);

    //    ;  I001/020: =0x 20
    //    ;  Target Report Descriptor: PLT ACT SEC A1

    loginf << "cat001 test: 020" << logendl;
    REQUIRE(record.at("020").at("TYP") == 0);
    REQUIRE(record.at("020").at("SIM") == 0);
    REQUIRE(record.at("020").at("SSR/PSR") == 2);
    REQUIRE(record.at("020").at("ANT") == 0);
    REQUIRE(record.at("020").at("SPI") == 0);
    REQUIRE(record.at("020").at("RAB") == 0);

    //    ;  I001/040: =0x 3f b8 b6 7c
    //    ;  Measured Position: rng=16312 (127.438 nmi); azm=46716 (256.619 deg)
    loginf << "cat001 test: 040" << logendl;
    REQUIRE(approximatelyEqual(record.at("040").at("RHO"), 127.4375, 10e-4));
    REQUIRE(approximatelyEqual(record.at("040").at("THETA"), 256.61865234375, 10e-10));

    //    ;  I001/070: =0x 0b 63
    //    ;  Mode 3/A Code: v=0; g=0; l=0; code=05543
    loginf << "cat001 test: 070" << logendl;
    REQUIRE(record.at("070").at("V") == 0);
    REQUIRE(record.at("070").at("G") == 0);
    REQUIRE(record.at("070").at("L") == 0);
    REQUIRE(record.at("070").at("Mode-3/A reply") == 5543);

    //    ;  I001/090: =0x 05 f0
    //    ;  Mode C Code: v=0; g=0; code=1520 (380.00 FL)
    loginf << "cat001 test: 090" << logendl;
    REQUIRE(record.at("090").at("V") == 0);
    REQUIRE(record.at("090").at("G") == 0);
    REQUIRE(record.at("090").at("Mode-C HEIGHT") == 38000.0);

    //    ;  I001/130: =0x c1 79 c0
    //    ;  Radar Plot Characteristics: 0x c1 79 c0
    // 193 -> 96, 121 -> 60, 192 -> 96
    loginf << "cat001 test: 130" << logendl;
    REQUIRE(record.at("130").at("Radar Plot Characteristics").size() == 3);
    REQUIRE(record.at("130").at("Radar Plot Characteristics")[0].at("Value") == 96);
    REQUIRE(record.at("130").at("Radar Plot Characteristics")[1].at("Value") == 60);
    REQUIRE(record.at("130").at("Radar Plot Characteristics")[2].at("Value") == 96);

    //    ;  I001/141: =0x 6e b7
    //    ;  Truncated Time of Day: 0x6eb7
    loginf << "cat001 test: 141" << logendl;
    REQUIRE(approximatelyEqual(record.at("141").at("Truncated Time of Day"), 221.4296875, 10e-6));

    //     [--:--:--.---] - --:--:--.--- -- 0x0001 A1 P:SSR ----- AR  256.619  127.438       A:05543
    //     --- C: 380    -- ;
}

TEST_CASE("jASTERIX CAT001 1.1", "[jASTERIX CAT001]")
{
    loginf << "cat001 test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, true, true, false);

    //      010014fe0001203fb8b67c0b6305f0c179c06eb7
    // echo -n 010014fe0001203fb8b67c0b6305f0c179c06eb7 | xxd -r -p > cat001ed1.1.bin

    //    const char *cat001_ed11 = "010014fe0001203fb8b67c0b6305f0c179c06eb7";
    //    char* target = new char[strlen(cat001_ed11)/2];

    //    size_t size = hex2bin (cat001_ed11, target);

    REQUIRE(jasterix.hasCategory(1));
    std::shared_ptr<jASTERIX::Category> cat001 = jasterix.category(1);
    REQUIRE(cat001->hasEdition("1.1"));
    cat001->setCurrentEdition("1.1");
    cat001->setCurrentMapping("");

    const std::string filename = "cat001ed1.1.bin";

    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));
    REQUIRE(jASTERIX::Files::fileSize(data_path + filename) == 20);

    jasterix.decodeFile(data_path + filename, test_cat001_callback);

    // delete[] target;

    loginf << "cat001 test: end" << logendl;
}
