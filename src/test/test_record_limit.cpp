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

#include <cstdio>

#include "catch.hpp"
#include "jasterix.h"
#include "logger.h"
#include "test_jasterix.h"
#include "test_synthetic_files.h"

using namespace std;
using namespace nlohmann;
using namespace synthetic;

// The global record_limit applies to decoding (structured and flat) and to analysis. It stops
// processing at the end of the chunk in which the limit is reached, so all modes see the same
// records and slightly more than the limit are delivered.
//
// Ten frames with one CAT247 record each, chunks of two frames, limit three records: the
// second chunk reaches the limit, so four records are processed in every mode.

namespace
{
// restores the global limits when the test case leaves, also on a failed REQUIRE
struct LimitGuard
{
    int frame_chunk_size = jASTERIX::frame_chunk_size;
    int data_block_chunk_size = jASTERIX::data_block_chunk_size;
    int record_limit = jASTERIX::record_limit;

    ~LimitGuard()
    {
        jASTERIX::frame_chunk_size = frame_chunk_size;
        jASTERIX::data_block_chunk_size = data_block_chunk_size;
        jASTERIX::record_limit = record_limit;
    }
};
}  // namespace

TEST_CASE("jASTERIX record limit IOSS", "[jASTERIX record limit]")
{
    loginf << "record limit ioss test: start" << logendl;

    LimitGuard guard;

    std::vector<unsigned char> file_bytes;
    for (unsigned int cnt = 0; cnt < 10; ++cnt)
        appendIOSSFrame(file_bytes, 0, 100.0 + cnt, cat247_data_block);

    std::string path = writeTempFile("record_limit_ioss", file_bytes);

    jASTERIX::frame_chunk_size = 2;
    jASTERIX::record_limit = 3;

    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);
    jasterix.category(247)->setCurrentEdition("1.2");

    SECTION("structured")
    {
        size_t sum_records = 0;
        size_t sum_frames = 0;

        jasterix.decodeFile(path, "ioss",
            [&](std::unique_ptr<json>, size_t, size_t num_frames, size_t num_records, size_t num_errors)
            {
                REQUIRE(num_errors == 0);
                sum_frames += num_frames;
                sum_records += num_records;
            },
            false);

        REQUIRE(sum_frames == 4);
        REQUIRE(sum_records == 4);
        REQUIRE(jasterix.numRecords() == 4);
    }

    SECTION("flat")
    {
        size_t sum_records = 0;
        size_t column_entries = 0;

        jasterix.decodeFile(path, "ioss",
            [&](std::unique_ptr<json> json_data, size_t, size_t, size_t num_records, size_t num_errors)
            {
                REQUIRE(num_errors == 0);
                sum_records += num_records;
                column_entries += json_data->at("247").at("010.SAC").size();
            },
            true);

        REQUIRE(sum_records == 4);
        REQUIRE(column_entries == 4);
    }

    SECTION("analyze")
    {
        std::unique_ptr<json> result = jasterix.analyzeFile(path, "ioss");

        REQUIRE(result->at("num_records") == 4);
        REQUIRE(result->at("num_errors") == 0);
    }

    SECTION("no limit")
    {
        jASTERIX::record_limit = -1;

        size_t sum_records = 0;

        jasterix.decodeFile(path, "ioss",
            [&](std::unique_ptr<json>, size_t, size_t, size_t num_records, size_t)
            { sum_records += num_records; },
            false);

        REQUIRE(sum_records == 10);
    }

    std::remove(path.c_str());

    loginf << "record limit ioss test: end" << logendl;
}

TEST_CASE("jASTERIX record limit PCAP", "[jASTERIX record limit]")
{
    loginf << "record limit pcap test: start" << logendl;

    LimitGuard guard;

    // ten packets, one data block each, chunks of two data blocks
    std::vector<unsigned char> file_bytes;
    appendPcapGlobalHeader(file_bytes);
    for (unsigned int cnt = 0; cnt < 10; ++cnt)
        appendPcapUDPPacket(file_bytes, utcSeconds(2026, 9, 16, 4, 0, cnt), 0, cat247_data_block);

    std::string path = writeTempFile("record_limit_pcap", file_bytes, ".pcap");

    jASTERIX::data_block_chunk_size = 2;
    jASTERIX::record_limit = 3;

    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);
    jasterix.category(247)->setCurrentEdition("1.2");

    SECTION("structured")
    {
        size_t sum_records = 0;

        jasterix.decodePCAPFile(path,
            [&](std::unique_ptr<json>, size_t, size_t, size_t num_records, size_t num_errors)
            {
                REQUIRE(num_errors == 0);
                sum_records += num_records;
            },
            false);

        REQUIRE(sum_records == 4);
        REQUIRE(jasterix.numRecords() == 4);
    }

    SECTION("flat")
    {
        size_t sum_records = 0;

        jasterix.decodePCAPFile(path,
            [&](std::unique_ptr<json> json_data, size_t, size_t, size_t num_records, size_t num_errors)
            {
                REQUIRE(num_errors == 0);
                REQUIRE(json_data->at("247").at("recording_time").size() == num_records);
                sum_records += num_records;
            },
            true);

        REQUIRE(sum_records == 4);
    }

    std::remove(path.c_str());

    loginf << "record limit pcap test: end" << logendl;
}
