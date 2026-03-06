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

void test_cat065_callback(std::unique_ptr<nlohmann::json> json_data, size_t total_num_bytes, size_t num_frames,
                          size_t num_records, size_t num_errors)
{
    loginf << "cat065 test: decoded " << num_frames << " frames, " << num_records << " records, "
           << num_errors << " errors" << logendl;

    REQUIRE(num_frames == 0);
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

    //    {
    //        "data_blocks": [
    //            {
    //                "category": 65,
    //                "content": {
    //                    "index": 3,
    //                    "length": 9,
    //                    "records": [
    //                        {
    //                            "000": {
    //                                "Message Type": 2
    //                            },
    //                            "010": {
    //                                "SAC": 0,
    //                                "SIC": 5
    //                            },
    //                            "015": {
    //                                "Service Identification": 193
    //                            },
    //                            "020": {
    //                                "Batch Number": 0
    //                            },
    //                            "030": {
    //                                "Time of Message": 33502.296875
    //                            },
    //                            "FSPEC": [
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                true,
    //                                false,
    //                                false,
    //                                false
    //                            ]
    //                        }
    //                    ]
    //                },
    //                "length": 12
    //            }
    //        ]
    //    }

    loginf << "cat065 test: data block" << logendl;

    REQUIRE(json_data->contains("data_blocks"));
    REQUIRE(json_data->at("data_blocks").is_array());
    REQUIRE(json_data->at("data_blocks").size() == 1);

    const json& first_data_block = json_data->at("data_blocks").at(0);

    REQUIRE(first_data_block.contains("category"));
    REQUIRE(first_data_block.at("category") == 65);
    REQUIRE(first_data_block.contains("length"));
    REQUIRE(first_data_block.at("length") == 12);

    loginf << "cat065 test: num records" << logendl;
    REQUIRE(first_data_block.contains("content"));
    REQUIRE(first_data_block.at("content").contains("records"));
    REQUIRE(first_data_block.at("content").at("records").is_array());
    REQUIRE(first_data_block.at("content").at("records").size() == 1);

    const json& record = first_data_block.at("content").at("records").at(0);

    //    ; FSPEC: 0x f8

    loginf << "cat065 test: fspec" << logendl;


    //    ; Data Record:
    //    ;  I065/010: =0x 00 05
    //    ;  Data Source Identifier: 0x0005 (SAC=0; SIC=5)

    loginf << "cat065 test: 010" << logendl;
    REQUIRE(record.at("010").at("SAC") == 0);
    REQUIRE(record.at("010").at("SIC") == 5);

    //    ;  I065/000: =0x 02
    //    ;  Message Type: 2 (end of batch)

    loginf << "cat065 test: 000" << logendl;
    REQUIRE(record.at("000").at("Message Type") == 02);

    //    ;  I065/015: =0x c1
    //    ;  Service Identification: 193

    loginf << "cat065 test: 015" << logendl;
    REQUIRE(record.at("015").at("Service Identification") == 193);

    //    ;  I065/030: =0x 41 6f 26
    //    ;  Time of Message: 0x416f26 (4288294; 09:18:22.297 UTC)

    loginf << "cat065 test: 030" << logendl;
    REQUIRE(approximatelyEqual(record.at("030").at("Time of Message"), 33502.296875, 10e-6));

    //    ;  I065/020: =0x 00
    //    ;  Batch Number: 0

    loginf << "cat065 test: 020" << logendl;
    REQUIRE(record.at("020").at("Batch Number") == 0);
}

TEST_CASE("jASTERIX CAT065 1.3", "[jASTERIX CAT065]")
{
    loginf << "cat065 test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, true, true, false);

    //    ASTERIX data block at pos 1133: cat=65; len=12
    //      41000cf8000502c1416f2600

    // echo -n 41000cf8000502c1416f2600 | xxd -r -p > cat065ed1.3.bin

    REQUIRE(jasterix.hasCategory(65));
    std::shared_ptr<jASTERIX::Category> cat065 = jasterix.category(65);
    REQUIRE(cat065->hasEdition("1.3"));
    cat065->setCurrentEdition("1.3");
    cat065->setCurrentMapping("");

    const std::string filename = "cat065ed1.3.bin";

    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));
    REQUIRE(jASTERIX::Files::fileSize(data_path + filename) == 12);

    jasterix.decodeFile(data_path + filename, test_cat065_callback);

    loginf << "cat065 test: end" << logendl;
}
