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

#include <fstream>
#include <vector>

#include "catch.hpp"
#include "files.h"
#include "jasterix.h"
#include "logger.h"
#include "string_conv.h"
#include "test_jasterix.h"

using namespace std;
using namespace nlohmann;

namespace
{

vector<char> readBinaryFile(const string& path)
{
    ifstream ifs(path, ios::binary | ios::ate);
    if (!ifs)
        throw runtime_error("cannot open file: " + path);

    size_t sz = static_cast<size_t>(ifs.tellg());
    ifs.seekg(0, ios::beg);

    vector<char> buf(sz);
    ifs.read(buf.data(), sz);
    return buf;
}

void roundtrip_test(const string& filename, unsigned int category,
                    const string& edition, size_t expected_file_size)
{
    loginf << "roundtrip test: cat" << category << " ed" << edition << " start" << logendl;

    string filepath = data_path + filename;
    REQUIRE(jASTERIX::Files::fileExists(filepath));
    REQUIRE(jASTERIX::Files::fileSize(filepath) == expected_file_size);

    // read original binary
    vector<char> original = readBinaryFile(filepath);
    REQUIRE(original.size() == expected_file_size);

    loginf << "original: " << bin2hex(original.data(), original.size()) << logendl;

    // set up jASTERIX
    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);

    REQUIRE(jasterix.hasCategory(category));
    auto cat = jasterix.category(category);
    REQUIRE(cat->hasEdition(edition));
    cat->setCurrentEdition(edition);
    cat->setCurrentMapping("");

    // decode file, capture JSON
    unique_ptr<json> decoded_json;

    jasterix.decodeFile(filepath,
                        [&](unique_ptr<json> json_data, size_t total_num_bytes, size_t num_frames,
                            size_t num_records, size_t num_errors)
                        {
                            REQUIRE(num_errors == 0);
                            REQUIRE(num_records > 0);
                            decoded_json = move(json_data);
                        });

    REQUIRE(decoded_json != nullptr);
    REQUIRE(decoded_json->contains("data_blocks"));

    const json& data_blocks = decoded_json->at("data_blocks");
    REQUIRE(data_blocks.is_array());
    REQUIRE(data_blocks.size() > 0);

    // re-encode all data blocks and concatenate
    vector<char> encoded;

    for (const auto& db : data_blocks)
    {
        REQUIRE(db.contains("category"));
        REQUIRE(db.at("category") == category);
        REQUIRE(db.contains("content"));
        REQUIRE(db.at("content").contains("records"));

        const json& records = db.at("content").at("records");
        REQUIRE(records.is_array());

        // collect record JSONs
        vector<json> record_vec;
        for (const auto& rec : records)
        {
            loginf << "record json '" << rec.dump() << "'";
            record_vec.push_back(rec);
        }

        // encode as data block
        vector<char> block = jasterix.encodeDataBlock(category, record_vec);

        encoded.insert(encoded.end(), block.begin(), block.end());

    }

    loginf << "encoded: " << bin2hex(encoded.data(), encoded.size()) << logendl;

    // compare byte-for-byte
    REQUIRE(encoded.size() == original.size());

    for (size_t i = 0; i < original.size(); ++i)
    {
        if (encoded[i] != original[i])
        {
            FAIL("byte mismatch at offset " << i
                 << ": expected 0x" << hex << (static_cast<unsigned>(original[i]) & 0xFF)
                 << " got 0x" << (static_cast<unsigned>(encoded[i]) & 0xFF));
        }
    }

    loginf << "roundtrip test: cat" << category << " ed" << edition
           << " passed (" << original.size() << " bytes)" << logendl;
}

}  // anonymous namespace

TEST_CASE("Roundtrip CAT001 1.1", "[encode]")
{
    roundtrip_test("cat001ed1.1.bin", 1, "1.1", 20);
}

TEST_CASE("Roundtrip CAT002 1.0", "[encode]")
{
    roundtrip_test("cat002ed1.0.bin", 2, "1.0", 12);
}

TEST_CASE("Roundtrip CAT004 1.4", "[encode]")
{
    roundtrip_test("cat004ed1.4.bin", 4, "1.4", 46);
}

TEST_CASE("Roundtrip CAT010 0.31", "[encode]")
{
    roundtrip_test("cat010ed0.31.bin", 10, "0.31", 41);
}

TEST_CASE("Roundtrip CAT010 0.24", "[encode]")
{
    roundtrip_test("cat010ed0.24_sensis.bin", 10, "0.24_sensis", 43);
}

TEST_CASE("Roundtrip CAT019 1.3", "[encode]")
{
    roundtrip_test("cat019ed1.3.bin", 19, "1.3", 57);
}

TEST_CASE("Roundtrip CAT020 1.5", "[encode]")
{
    roundtrip_test("cat020ed1.5.bin", 20, "1.5", 101);
}

TEST_CASE("Roundtrip CAT021 0.26", "[encode]")
{
    roundtrip_test("cat021ed0.26.bin", 21, "0.26", 36);
}

TEST_CASE("Roundtrip CAT021 2.1", "[encode]")
{
    roundtrip_test("cat021ed2.1.bin", 21, "2.1", 49);
}

TEST_CASE("Roundtrip CAT030 7.0", "[encode]")
{
    roundtrip_test("cat030ed7.0.bin", 30, "7.0", 64);
}

TEST_CASE("Roundtrip CAT034 1.26", "[encode]")
{
    roundtrip_test("cat034ed1.26.bin", 34, "1.26", 20);
}

TEST_CASE("Roundtrip CAT048 1.15", "[encode]")
{
    roundtrip_test("cat048ed1.15.bin", 48, "1.15", 65);
}

TEST_CASE("Roundtrip CAT048 1.23", "[encode]")
{
    roundtrip_test("cat048ed1.23.bin", 48, "1.23", 72);
}

TEST_CASE("Roundtrip CAT062 1.12", "[encode]")
{
    roundtrip_test("cat062ed1.12.bin", 62, "1.12", 150);
}

TEST_CASE("Roundtrip CAT062 1.16", "[encode]")
{
    roundtrip_test("cat062ed1.16.bin", 62, "1.16", 63);
}

TEST_CASE("Roundtrip CAT063 1.0", "[encode]")
{
    roundtrip_test("cat063ed1.0.bin", 63, "1.0", 30);
}

TEST_CASE("Roundtrip CAT065 1.3", "[encode]")
{
    roundtrip_test("cat065ed1.3.bin", 65, "1.3", 12);
}

TEST_CASE("Roundtrip CAT247 1.2", "[encode]")
{
    roundtrip_test("cat247ed1.2.bin", 247, "1.2", 20);
}

TEST_CASE("Roundtrip CAT252 7.0", "[encode]")
{
    roundtrip_test("cat252ed7.0.bin", 252, "7.0", 16);
}
