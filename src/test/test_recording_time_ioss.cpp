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
#include <fstream>
#include <vector>

#include "catch.hpp"
#include "jasterix.h"
#include "logger.h"
#include "test_jasterix.h"
#include "test_synthetic_files.h"

using namespace std;
using namespace nlohmann;

// IOSS frames carry recording_day (day counter) and recording_time (seconds since midnight,
// 10 ms resolution) in the frame header. Both must show up in every data block of the frame
// in structured output and as per-record side columns in flat output.
//
// The test file is built in memory: two frames, each with the CAT247 data block of
// test_cat247_1.2.cpp, on two different recording days.

using namespace synthetic;

TEST_CASE("jASTERIX IOSS recording time", "[jASTERIX recording time]")
{
    loginf << "ioss recording time test: start" << logendl;

    std::vector<unsigned char> file_bytes;
    appendIOSSFrame(file_bytes, 0, 14432.0, cat247_data_block);
    appendIOSSFrame(file_bytes, 1, 86399.99, cat247_data_block);

    std::string path = writeTempFile("ioss_recording_time", file_bytes);

    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);

    REQUIRE(jasterix.hasCategory(247));
    jasterix.category(247)->setCurrentEdition("1.2");

    SECTION("structured: frame and data block keys")
    {
        json all_frames = json::array();

        jasterix.decodeFile(path, "ioss",
            [&](std::unique_ptr<json> json_data, size_t, size_t, size_t num_records, size_t num_errors)
            {
                REQUIRE(num_errors == 0);
                REQUIRE(num_records == 2);
                REQUIRE(json_data->contains("frames"));

                for (const json& frame : json_data->at("frames"))
                    all_frames.push_back(frame);
            },
            false);

        REQUIRE(all_frames.size() == 2);

        const json& frame0 = all_frames.at(0);
        const json& frame1 = all_frames.at(1);

        // the old item name is gone
        REQUIRE(!frame0.contains("time_ms"));

        REQUIRE(approximatelyEqual(frame0.at("recording_time"), 14432.0, 1e-6));
        REQUIRE(frame0.at("recording_day") == 0);
        REQUIRE(!frame0.contains("recording_date"));

        REQUIRE(approximatelyEqual(frame1.at("recording_time"), 86399.99, 1e-6));
        REQUIRE(frame1.at("recording_day") == 1);

        // every data block of the frame carries the frame's recording time
        const json& data_block0 = frame0.at("content").at("data_blocks").at(0);
        const json& data_block1 = frame1.at("content").at("data_blocks").at(0);

        REQUIRE(data_block0.at("category") == 247);
        REQUIRE(approximatelyEqual(data_block0.at("recording_time"), 14432.0, 1e-6));
        REQUIRE(data_block0.at("recording_day") == 0);
        REQUIRE(!data_block0.contains("recording_date"));

        REQUIRE(approximatelyEqual(data_block1.at("recording_time"), 86399.99, 1e-6));
        REQUIRE(data_block1.at("recording_day") == 1);

        // record content is untouched
        const json& record = data_block0.at("content").at("records").at(0);
        REQUIRE(record.at("010").at("SAC") == 0);
        REQUIRE(record.at("010").at("SIC") == 1);
        REQUIRE(!record.contains("recording_time"));
    }

    SECTION("flat: per-record side columns")
    {
        json cat247_columns;
        size_t sum_records = 0;

        jasterix.decodeFile(path, "ioss",
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
        REQUIRE(cat247_columns.contains("recording_day"));
        REQUIRE(!cat247_columns.contains("recording_date"));  // IOSS has no absolute date

        const json& time_col = cat247_columns.at("recording_time");
        const json& day_col = cat247_columns.at("recording_day");

        REQUIRE(time_col.size() == 2);
        REQUIRE(day_col.size() == 2);
        REQUIRE(approximatelyEqual(time_col.at(0), 14432.0, 1e-6));
        REQUIRE(approximatelyEqual(time_col.at(1), 86399.99, 1e-6));
        REQUIRE(day_col.at(0) == 0);
        REQUIRE(day_col.at(1) == 1);

        // aligned with the leaf columns
        REQUIRE(cat247_columns.at("010.SAC").size() == 2);
    }

    std::remove(path.c_str());

    loginf << "ioss recording time test: end" << logendl;
}
