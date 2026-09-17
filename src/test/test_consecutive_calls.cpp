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

// One jASTERIX instance is used for several decode or analyze calls in a row, as COMPASS and
// analyzePCAPFile (one analyzeData per network stream) do. Every call must see all of its
// own data and nothing of the earlier calls: the chunk pipeline state (done flags, leftover
// chunks of a force-stopped task) belongs to one call only.
//
// Found with a PCAP whose second stream got 0 records without a record limit, and stale data
// blocks of the previous stream with one.

namespace
{
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

std::vector<unsigned char> rawDataBlocks(unsigned int count)
{
    std::vector<unsigned char> bytes;
    for (unsigned int cnt = 0; cnt < count; ++cnt)
        bytes.insert(bytes.end(), cat247_data_block.begin(), cat247_data_block.end());
    return bytes;
}

size_t decodeRawRecords(jASTERIX::jASTERIX& jasterix, const std::vector<unsigned char>& bytes,
                        bool flat)
{
    size_t sum_records = 0;

    jasterix.decodeData(reinterpret_cast<const char*>(bytes.data()), bytes.size(),
        [&](std::unique_ptr<json>, size_t, size_t, size_t num_records, size_t num_errors)
        {
            REQUIRE(num_errors == 0);
            sum_records += num_records;
        },
        true, flat);

    return sum_records;
}

size_t decodeFramedRecords(jASTERIX::jASTERIX& jasterix, const std::string& path, bool flat)
{
    size_t sum_records = 0;

    jasterix.decodeFile(path, "ioss",
        [&](std::unique_ptr<json>, size_t, size_t, size_t num_records, size_t num_errors)
        {
            REQUIRE(num_errors == 0);
            sum_records += num_records;
        },
        flat);

    return sum_records;
}
}  // namespace

TEST_CASE("jASTERIX consecutive calls on one instance", "[jASTERIX consecutive calls]")
{
    loginf << "consecutive calls test: start" << logendl;

    LimitGuard guard;

    std::vector<unsigned char> raw = rawDataBlocks(10);

    std::vector<unsigned char> framed;
    for (unsigned int cnt = 0; cnt < 10; ++cnt)
        appendIOSSFrame(framed, 0, 100.0 + cnt, cat247_data_block);
    std::string framed_path = writeTempFile("consecutive_ioss", framed);

    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);
    jasterix.category(247)->setCurrentEdition("1.2");

    SECTION("raw: analyze, decode structured, decode flat, twice each")
    {
        for (unsigned int round = 0; round < 2; ++round)
        {
            std::unique_ptr<json> analysis = jasterix.analyzeData(
                reinterpret_cast<const char*>(raw.data()), raw.size());
            REQUIRE(analysis->at("num_records") == 10);
            REQUIRE(analysis->at("num_errors") == 0);

            REQUIRE(decodeRawRecords(jasterix, raw, false) == 10);
            REQUIRE(decodeRawRecords(jasterix, raw, true) == 10);
        }
    }

    SECTION("framed: analyze, decode structured, decode flat, twice each")
    {
        for (unsigned int round = 0; round < 2; ++round)
        {
            std::unique_ptr<json> analysis = jasterix.analyzeFile(framed_path, "ioss");
            REQUIRE(analysis->at("num_records") == 10);
            REQUIRE(analysis->at("num_errors") == 0);

            REQUIRE(decodeFramedRecords(jasterix, framed_path, false) == 10);
            REQUIRE(decodeFramedRecords(jasterix, framed_path, true) == 10);
        }
    }

    SECTION("record limit stops, repeated: no stale chunks, no lost calls")
    {
        // small chunks and a limit force-stop the producer task in every call
        jASTERIX::data_block_chunk_size = 2;
        jASTERIX::frame_chunk_size = 2;
        jASTERIX::record_limit = 3;

        for (unsigned int round = 0; round < 20; ++round)
        {
            std::unique_ptr<json> analysis = jasterix.analyzeData(
                reinterpret_cast<const char*>(raw.data()), raw.size());
            REQUIRE(analysis->at("num_errors") == 0);
            REQUIRE(analysis->at("num_records") == 4);

            REQUIRE(decodeRawRecords(jasterix, raw, false) == 4);
            REQUIRE(decodeFramedRecords(jasterix, framed_path, true) == 4);
        }
    }

    std::remove(framed_path.c_str());

    loginf << "consecutive calls test: end" << logendl;
}

TEST_CASE("jASTERIX analyze PCAP with several streams", "[jASTERIX consecutive calls]")
{
    loginf << "analyze pcap streams test: start" << logendl;

    LimitGuard guard;

    // two network streams (different destination ports), 10 data blocks each
    std::vector<unsigned char> file_bytes;
    appendPcapGlobalHeader(file_bytes);
    for (unsigned int cnt = 0; cnt < 10; ++cnt)
    {
        appendPcapUDPPacket(file_bytes, utcSeconds(2026, 9, 16, 4, 0, cnt), 0, cat247_data_block);
        appendPcapUDPPacket(file_bytes, utcSeconds(2026, 9, 16, 4, 0, cnt), 500000,
                            cat247_data_block, 5679);
    }
    std::string path = writeTempFile("analyze_pcap_streams", file_bytes, ".pcap");

    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);
    jasterix.category(247)->setCurrentEdition("1.2");

    auto stream_records = [](const json& result) {
        std::vector<std::pair<size_t, size_t>> records_errors;
        for (const auto& item : result.items())
            if (item.value().is_object() && item.value().contains("num_records"))
                records_errors.emplace_back(item.value().at("num_records").get<size_t>(),
                                            item.value().at("num_errors").get<size_t>());
        return records_errors;
    };

    SECTION("no limit: every stream complete")
    {
        std::unique_ptr<json> result = jasterix.analyzePCAPFile(path);
        auto streams = stream_records(*result);

        REQUIRE(streams.size() == 2);
        for (const auto& [records, errors] : streams)
        {
            REQUIRE(records == 10);
            REQUIRE(errors == 0);
        }
    }

    SECTION("record limit: every stream stopped at the same point, no errors")
    {
        jASTERIX::data_block_chunk_size = 2;
        jASTERIX::record_limit = 3;

        std::unique_ptr<json> result = jasterix.analyzePCAPFile(path);
        auto streams = stream_records(*result);

        REQUIRE(streams.size() == 2);
        for (const auto& [records, errors] : streams)
        {
            REQUIRE(records == 4);
            REQUIRE(errors == 0);
        }
    }

    std::remove(path.c_str());

    loginf << "analyze pcap streams test: end" << logendl;
}
