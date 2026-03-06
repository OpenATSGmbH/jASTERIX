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

void test_cat021_026_callback(std::unique_ptr<nlohmann::json> json_data, size_t total_num_bytes, size_t num_frames,
                          size_t num_records, size_t num_errors)
{
    loginf << "cat021 test: decoded " << num_frames << " frames, " << num_records << " records, "
           << num_errors << " errors" << logendl;

    REQUIRE(num_frames == 0);
    REQUIRE(num_records == 1);
    REQUIRE(num_errors == 0);

    // {
    //     "data_blocks": [
    //         {
    //             "category": 21,
    //             "content": {
    //                 "index": 3,
    //                 "length": 33,
    //                 "records": [
    //                     {
    //                         "010": {
    //                             "SAC": 0,
    //                             "SIC": 5
    //                         },
    //                         "020": {
    //                             "ECAT": 21
    //                         },
    //                         "030": {
    //                             "Time of Day": 14418.484375
    //                         },
    //                         "040": {
    //                             "ARC": 2,
    //                             "ATP": 1,
    //                             "DCR": 0,
    //                             "GBS": 1,
    //                             "RAB": 0,
    //                             "SAA": 0,
    //                             "SIM": 0,
    //                             "SPI": 0,
    //                             "TST": 0
    //                         },
    //                         "080": {
    //                             "Target Address": 4811822
    //                         },
    //                         "090": {
    //                             "AC": 0,
    //                             "DC": 0,
    //                             "MN": 0,
    //                             "PA": 8
    //                         },
    //                         "130": {
    //                             "Latitude": 38.777323149905996,
    //                             "Longitude": -9.131237217748
    //                         },
    //                         "170": {
    //                             "Target Identification": "FM012   "
    //                         },
    //                         "200": {
    //                             "Target Status": 0
    //                         },
    //                         "210": {
    //                             "DTI": 0,
    //                             "MDS": 1,
    //                             "OTR": 0,
    //                             "UAT": 0,
    //                             "VDL": 0
    //                         },
    //                         "FSPEC": [
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
    //                             false,
    //                             false,
    //                             false,
    //                             true,
    //                             false,
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
    //                             false
    //                         ],
    //                         "index": 3,
    //                         "length": 33,
    //                         "record_data": "fb811380000540301c293e006e4cc9ffe606d6496c2e00080818dc31ca08200015"
    //                     }
    //                 ]
    //             },
    //             "length": 36
    //         }
    //     ],
    //     "rec_num": 0
    // }

    loginf << "cat021 test: data block" << logendl;

    REQUIRE(json_data->contains("data_blocks"));
    REQUIRE(json_data->at("data_blocks").is_array());
    REQUIRE(json_data->at("data_blocks").size() == 1);

    const json& first_data_block = json_data->at("data_blocks").at(0);

    REQUIRE(first_data_block.contains("category"));
    REQUIRE(first_data_block.at("category") == 21);
    REQUIRE(first_data_block.contains("length"));
    REQUIRE(first_data_block.at("length") == 36);

    loginf << "cat021 test: num records" << logendl;
    REQUIRE(first_data_block.contains("content"));
    REQUIRE(first_data_block.at("content").contains("records"));
    REQUIRE(first_data_block.at("content").at("records").is_array());
    REQUIRE(first_data_block.at("content").at("records").size() == 1);

    const json& record = first_data_block.at("content").at("records").at(0);

    //    ; FSPEC: 0x fb 81 13 80
    // 11111011 10000001 00010011 10000000

    loginf << "cat021 test: fspec" << logendl;


    //    ;  I021/010: =0x 00 05
    //    ;  Data Source Identifier: 0x0005 (SAC=0; SIC=5)

    loginf << "cat021 test: 010" << logendl;
    REQUIRE(record.at("010").at("SAC") == 0);
    REQUIRE(record.at("010").at("SIC") == 5);

    //    ;  I021/040: =0x 40 30
    //    ;  Target Report Descriptor:
    //    ;   GBS
    //    ;   ATP=1 (24-bit ICAO address) ARC=2 (100 ft)

    loginf << "cat021 test: 040" << logendl;
    REQUIRE(record.at("040").at("ARC") == 2);
    REQUIRE(record.at("040").at("ATP") == 1);
    REQUIRE(record.at("040").at("DCR") == 0);
    REQUIRE(record.at("040").at("GBS") == 1);
    REQUIRE(record.at("040").at("RAB") == 0);
    REQUIRE(record.at("040").at("SAA") == 0);
    REQUIRE(record.at("040").at("SIM") == 0);
    REQUIRE(record.at("040").at("SPI") == 0);
    REQUIRE(record.at("040").at("TST") == 0);

    //    ;  I021/030: =0x 1c 29 3e
    //    ;  Time of Day: 0x1c293e (1845566; 04:00:18.484 UTC)

    loginf << "cat021 test: 030" << logendl;
    REQUIRE(approximatelyEqual(record.at("030").at("Time of Day"), 14418.484375, 10e-6));

    //    ;  I021/130: =0x 00 6e 4c c9 ff e6 06 d6
    //    ;  Position in WGS-84 Co-ordinates:
    //    ;   lat=7228617 (38:46:38.363N); lon=-1702186 (009:07:52.454W)

    loginf << "cat021 test: 130" << logendl;
    REQUIRE(approximatelyEqual(record.at("130").at("Latitude"), 38.77732336521148681641, 10e-15));
    REQUIRE(approximatelyEqual(record.at("130").at("Longitude"), -9.131237268447875976563, 10e-12));

    //    ;  I021/080: =0x 49 6c 2e
    //    ;  Aircraft Address: 0x496c2e (4811822)

    loginf << "cat021 test: 080" << logendl;
    REQUIRE(record.at("080").at("Target Address") == 4811822);

    //    ;  I021/090: =0x 00 08
    //    ;  Figure of Merit: ac=0; mn=0; dc=0; pa=8

    loginf << "cat021 test: 090" << logendl;
    REQUIRE(record.at("090").at("AC") == 0);
    REQUIRE(record.at("090").at("DC") == 0);
    REQUIRE(record.at("090").at("MN") == 0);
    REQUIRE(record.at("090").at("PA") == 8);

    //    ;  I021/210: =0x 08
    //    ;  Link Technology Indicator: MDS

    loginf << "cat021 test: 210" << logendl;
    REQUIRE(record.at("210").at("DTI") == 0);
    REQUIRE(record.at("210").at("MDS") == 1);
    REQUIRE(record.at("210").at("OTR") == 0);
    REQUIRE(record.at("210").at("UAT") == 0);
    REQUIRE(record.at("210").at("VDL") == 0);

    //    ;  I021/170: =0x 18 dc 31 ca 08 20
    //    ;  Target Identification: idt=[FM012   ]

    loginf << "cat021 test: 170" << logendl;
    REQUIRE(record.at("170").at("Target Identification") == "FM012   ");

    //    ;  I021/200: =0x 00
    //    ;  Target Status: 0

    loginf << "cat021 test: 200" << logendl;
    REQUIRE(record.at("200").at("Target Status") == 0);

    //    ;  I021/020: =0x 15
    //    ;  Emitter Category: 21 (surface service vehicle)

    loginf << "cat021 test: 020" << logendl;
    REQUIRE(record.at("020").at("ECAT") == 21);
}

TEST_CASE("jASTERIX CAT021 0.26", "[jASTERIX CAT021]")
{
    loginf << "cat021 test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, true, true, false);

    //; ASTERIX data block at pos 0: cat=21; len=36
    //  150024fb811380000540301c293e006e4cc9ffe606d6496c2e00080818dc31ca08200015

    // echo -n
    // 150024fb811380000540301c293e006e4cc9ffe606d6496c2e00080818dc31ca08200015
    // | xxd -r -p > cat021ed0.26.bin

    REQUIRE(jasterix.hasCategory(21));
    std::shared_ptr<jASTERIX::Category> cat021 = jasterix.category(21);
    REQUIRE(cat021->hasEdition("0.26"));
    cat021->setCurrentEdition("0.26");
    cat021->setCurrentMapping("");

    const std::string filename = "cat021ed0.26.bin";

    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));
    REQUIRE(jASTERIX::Files::fileSize(data_path + filename) == 36);

    jasterix.decodeFile(data_path + filename, test_cat021_026_callback);

    loginf << "cat021 test: end" << logendl;
}
