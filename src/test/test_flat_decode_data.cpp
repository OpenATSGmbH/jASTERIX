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
#include "test_jasterix.h"

#include <fstream>
#include <vector>

using namespace std;
using namespace nlohmann;

namespace
{

std::vector<char> read_file(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    return std::vector<char>((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
}

// every column of a flat category object must hold one entry per record
void require_columns_aligned(const json& cat_columns, size_t num_records)
{
    REQUIRE(cat_columns.is_object());

    for (auto it = cat_columns.begin(); it != cat_columns.end(); ++it)
    {
        REQUIRE(it.value().is_array());

        if (it.value().size() != num_records)
            logerr << "column '" << it.key() << "' has " << it.value().size()
                   << " entries, expected " << num_records << logendl;

        REQUIRE(it.value().size() == num_records);
    }
}

}  // anonymous namespace

// decodeData is a separate code path from decodeFile and used by the PCAP and network
// readers. It must set up the flat state completely, otherwise the corrections that need
// the column data (CAT001 SAC/SIC propagation, CAT001 time reconstruction) are skipped.
TEST_CASE("jASTERIX flat decodeData CAT001 SAC/SIC propagation", "[jASTERIX flat decodeData]")
{
    loginf << "flat decodeData sac/sic test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);

    REQUIRE(jasterix.hasCategory(1));
    std::shared_ptr<jASTERIX::Category> cat001 = jasterix.category(1);
    REQUIRE(cat001->hasEdition("1.1"));
    cat001->setCurrentEdition("1.1");

    const std::string filename = "cat001ed1.1_sacsic_propagation.bin";
    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));

    std::vector<char> data = read_file(data_path + filename);
    REQUIRE(data.size() == 50);

    bool callback_called = false;

    jasterix.decodeData(data.data(), (unsigned int)data.size(),
        [&](std::unique_ptr<nlohmann::json> json_data, size_t total_num_bytes,
            size_t num_frames, size_t num_records, size_t num_errors)
        {
            callback_called = true;

            REQUIRE(num_records == 3);
            REQUIRE(num_errors == 0);

            REQUIRE(json_data->contains("1"));
            const json& cat1 = json_data->at("1");

            REQUIRE(cat1.contains("010.SAC"));
            REQUIRE(cat1.contains("010.SIC"));

            // records 2 and 3 omit I001/010, the values must be propagated from record 1
            for (size_t i = 0; i < 3; ++i)
            {
                REQUIRE(cat1.at("010.SAC").at(i) == 0);
                REQUIRE(cat1.at("010.SIC").at(i) == 1);
            }
        },
        false,   // not abortable
        true);   // flat

    REQUIRE(callback_called);

    loginf << "flat decodeData sac/sic test: end" << logendl;
}

// The side columns (artas_md5, record_data) grow by push_back while the leaf columns are
// written at the record index. Both must end up with one entry per accepted record.
TEST_CASE("jASTERIX flat column alignment", "[jASTERIX flat decodeData]")
{
    loginf << "flat column alignment test: start" << logendl;

    jASTERIX::add_record_data = true;

    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);

    REQUIRE(jasterix.hasCategory(1));
    std::shared_ptr<jASTERIX::Category> cat001 = jasterix.category(1);
    REQUIRE(cat001->hasEdition("1.1"));
    cat001->setCurrentEdition("1.1");

    const std::string filename = "cat001ed1.1_sacsic_propagation.bin";
    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));

    std::vector<char> data = read_file(data_path + filename);

    SECTION("well formed data block")
    {
        bool callback_called = false;

        jasterix.decodeData(data.data(), (unsigned int)data.size(),
            [&](std::unique_ptr<nlohmann::json> json_data, size_t total_num_bytes,
                size_t num_frames, size_t num_records, size_t num_errors)
            {
                callback_called = true;

                REQUIRE(num_records == 3);
                REQUIRE(num_errors == 0);
                REQUIRE(json_data->contains("1"));

                const json& cat1 = json_data->at("1");
                REQUIRE(cat1.contains("record_data"));
                require_columns_aligned(cat1, num_records);
            },
            false, true);

        REQUIRE(callback_called);
    }

    SECTION("data block length one byte short")
    {
        // shorten the declared data block length, so the last record does not fit
        std::vector<char> broken = data;
        REQUIRE(broken.size() > 3);

        unsigned int block_length = ((unsigned char)broken[1] << 8) | (unsigned char)broken[2];
        REQUIRE(block_length > 1);

        --block_length;
        broken[1] = (char)((block_length >> 8) & 0xff);
        broken[2] = (char)(block_length & 0xff);

        bool callback_called = false;

        jasterix.decodeData(broken.data(), (unsigned int)broken.size(),
            [&](std::unique_ptr<nlohmann::json> json_data, size_t total_num_bytes,
                size_t num_frames, size_t num_records, size_t num_errors)
            {
                callback_called = true;

                REQUIRE(json_data->contains("1"));

                const json& cat1 = json_data->at("1");
                REQUIRE(cat1.contains("record_data"));

                // whatever the decoder accepted, all columns must describe the same records
                require_columns_aligned(cat1, num_records);
            },
            false, true);

        REQUIRE(callback_called);
    }

    jASTERIX::add_record_data = false;

    loginf << "flat column alignment test: end" << logendl;
}
