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

#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <vector>

#include "catch.hpp"
#include "jasterix.h"
#include "logger.h"
#include "test_jasterix.h"
#include "test_synthetic_files.h"

using namespace std;
using namespace nlohmann;

// PCAP: the capture timestamp of the packet gives recording_time (seconds since UTC
// midnight) and recording_date (UTC, YYYYMMDD) per data block. There is no day counter.
//
// The capture is built in memory: raw IP link type (no Ethernet header), two UDP packets
// with the CAT247 data block of test_cat247_1.2.cpp, captured on two different UTC days.

using namespace synthetic;

TEST_CASE("jASTERIX PCAP recording time", "[jASTERIX recording time]")
{
    loginf << "pcap recording time test: start" << logendl;

    // 2026-09-16 04:00:32.5 UTC and 2026-09-17 00:00:00.25 UTC
    std::vector<unsigned char> file_bytes;
    appendPcapGlobalHeader(file_bytes);
    appendPcapUDPPacket(file_bytes, utcSeconds(2026, 9, 16, 4, 0, 32), 500000, cat247_data_block);
    appendPcapUDPPacket(file_bytes, utcSeconds(2026, 9, 17, 0, 0, 0), 250000, cat247_data_block);

    std::string path = writeTempFile("pcap_recording_time", file_bytes, ".pcap");

    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);

    REQUIRE(jasterix.hasCategory(247));
    jasterix.category(247)->setCurrentEdition("1.2");

    SECTION("structured: data block keys")
    {
        json all_data_blocks = json::array();

        jasterix.decodePCAPFile(path,
            [&](std::unique_ptr<json> json_data, size_t, size_t, size_t, size_t num_errors)
            {
                REQUIRE(num_errors == 0);
                REQUIRE(json_data->contains("data_blocks"));

                for (const json& data_block : json_data->at("data_blocks"))
                    all_data_blocks.push_back(data_block);
            },
            false);

        REQUIRE(all_data_blocks.size() == 2);

        const json& data_block0 = all_data_blocks.at(0);
        const json& data_block1 = all_data_blocks.at(1);

        // existing PCAP keys stay
        REQUIRE(data_block0.contains("pcap_time"));
        REQUIRE(data_block0.contains("pcap_time_epoch"));

        REQUIRE(approximatelyEqual(data_block0.at("recording_time"), 14432.5, 1e-6));
        REQUIRE(data_block0.at("recording_date") == 20260916);
        REQUIRE(!data_block0.contains("recording_day"));

        REQUIRE(approximatelyEqual(data_block1.at("recording_time"), 0.25, 1e-6));
        REQUIRE(data_block1.at("recording_date") == 20260917);

        REQUIRE(data_block0.at("content").at("records").at(0).at("010").at("SIC") == 1);
    }

    SECTION("flat: per-record side columns")
    {
        json cat247_columns;
        size_t sum_records = 0;

        jasterix.decodePCAPFile(path,
            [&](std::unique_ptr<json> json_data, size_t, size_t, size_t num_records, size_t num_errors)
            {
                REQUIRE(num_errors == 0);
                sum_records += num_records;

                REQUIRE(json_data->contains("247"));
                cat247_columns = json_data->at("247");
            },
            true);

        REQUIRE(sum_records == 2);

        REQUIRE(cat247_columns.contains("recording_time"));
        REQUIRE(cat247_columns.contains("recording_date"));
        REQUIRE(!cat247_columns.contains("recording_day"));

        const json& time_col = cat247_columns.at("recording_time");
        const json& date_col = cat247_columns.at("recording_date");

        REQUIRE(time_col.size() == 2);
        REQUIRE(approximatelyEqual(time_col.at(0), 14432.5, 1e-6));
        REQUIRE(approximatelyEqual(time_col.at(1), 0.25, 1e-6));
        REQUIRE(date_col.at(0) == 20260916);
        REQUIRE(date_col.at(1) == 20260917);

        REQUIRE(cat247_columns.at("010.SAC").size() == 2);
    }

    std::remove(path.c_str());

    loginf << "pcap recording time test: end" << logendl;
}
