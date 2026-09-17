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
#include "frameparser.h"
#include "jasterix.h"
#include "logger.h"
#include "test_jasterix.h"
#include "test_synthetic_files.h"

using namespace std;
using namespace nlohmann;

// RFF: 128 byte file header with start and stop wall time as text, then frames with a
// 4 byte relative time in ms and a 2 byte length, both little-endian. The recording time
// of a frame is the header start time plus the relative time, wrapped at midnight with a
// day counter, and the UTC date follows from the header start date.
//
// The test files are built in memory around the CAT247 data block of test_cat247_1.2.cpp.

using namespace synthetic;

TEST_CASE("jASTERIX RFF wall time parsing", "[jASTERIX recording time]")
{
    int y, mo, d, h, mi, s;

    // "YYYY-MM-DD HH:MM:SS"
    REQUIRE(jASTERIX::FrameParser::parseWallTime("2026-09-16 23:59:58", y, mo, d, h, mi, s));
    REQUIRE(y == 2026);
    REQUIRE(mo == 9);
    REQUIRE(d == 16);
    REQUIRE(h == 23);
    REQUIRE(mi == 59);
    REQUIRE(s == 58);

    // US style " MM/DD/YY HH:MM:SS", leading space
    REQUIRE(jASTERIX::FrameParser::parseWallTime(" 09/16/26 04:00:31", y, mo, d, h, mi, s));
    REQUIRE(y == 2026);
    REQUIRE(mo == 9);
    REQUIRE(d == 16);
    REQUIRE(h == 4);
    REQUIRE(mi == 0);
    REQUIRE(s == 31);

    // German style "DD/MM/YY HH:MM:SS", two-digit year >= 70 is 19xx
    REQUIRE(jASTERIX::FrameParser::parseWallTime("16/09/99 12:30:00", y, mo, d, h, mi, s));
    REQUIRE(y == 1999);
    REQUIRE(mo == 9);
    REQUIRE(d == 16);
    REQUIRE(h == 12);

    // German style with swapped day and month
    REQUIRE(jASTERIX::FrameParser::parseWallTime("09/16/26 12:30:00", y, mo, d, h, mi, s));
    REQUIRE(mo == 9);
    REQUIRE(d == 16);

    // rejected texts
    REQUIRE(!jASTERIX::FrameParser::parseWallTime("no time here", y, mo, d, h, mi, s));
    REQUIRE(!jASTERIX::FrameParser::parseWallTime("", y, mo, d, h, mi, s));
    REQUIRE(!jASTERIX::FrameParser::parseWallTime("2026-13-01 00:00:00", y, mo, d, h, mi, s));
    REQUIRE(!jASTERIX::FrameParser::parseWallTime("2026-09-16 24:00:00", y, mo, d, h, mi, s));

    // date arithmetic across month and year end
    REQUIRE(jASTERIX::FrameParser::dateYYYYMMDD(2026, 9, 16, 0) == 20260916);
    REQUIRE(jASTERIX::FrameParser::dateYYYYMMDD(2026, 9, 16, 1) == 20260917);
    REQUIRE(jASTERIX::FrameParser::dateYYYYMMDD(2026, 12, 31, 1) == 20270101);
    REQUIRE(jASTERIX::FrameParser::dateYYYYMMDD(2028, 2, 28, 1) == 20280229);
}

TEST_CASE("jASTERIX RFF recording time", "[jASTERIX recording time]")
{
    loginf << "rff recording time test: start" << logendl;

    // recording starts 2 s before midnight, frame 1 at +0.5 s, frame 2 at +2.5 s (next day)
    std::vector<unsigned char> file_bytes;
    appendRFFHeader(file_bytes, "2026-09-16 23:59:58", "2026-09-17 00:00:10");
    appendRFFFrame(file_bytes, 500, cat247_data_block);
    appendRFFFrame(file_bytes, 2500, cat247_data_block);

    std::string path = writeTempFile("rff_recording_time", file_bytes);

    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);

    REQUIRE(jasterix.hasCategory(247));
    jasterix.category(247)->setCurrentEdition("1.2");

    SECTION("structured: header, frame and data block keys")
    {
        json header;
        json all_frames = json::array();

        jasterix.decodeFile(path, "rff",
            [&](std::unique_ptr<json> json_data, size_t, size_t, size_t num_records, size_t num_errors)
            {
                REQUIRE(num_errors == 0);
                REQUIRE(num_records == 2);

                header = *json_data;
                header.erase("frames");

                for (const json& frame : json_data->at("frames"))
                    all_frames.push_back(frame);
            },
            false);

        // file header: raw text plus parsed start time and date
        REQUIRE(header.at("start_datetime") == "2026-09-16 23:59:58");
        REQUIRE(approximatelyEqual(header.at("recording_start_time"), 86398.0, 1e-6));
        REQUIRE(header.at("recording_start_date") == 20260916);

        REQUIRE(all_frames.size() == 2);

        const json& frame0 = all_frames.at(0);
        const json& frame1 = all_frames.at(1);

        // little-endian relative time (byte order fix)
        REQUIRE(frame0.at("frame_relative_time_ms") == 500);
        REQUIRE(frame1.at("frame_relative_time_ms") == 2500);

        REQUIRE(approximatelyEqual(frame0.at("recording_time"), 86398.5, 1e-6));
        REQUIRE(frame0.at("recording_day") == 0);
        REQUIRE(frame0.at("recording_date") == 20260916);

        // wrapped past midnight
        REQUIRE(approximatelyEqual(frame1.at("recording_time"), 0.5, 1e-6));
        REQUIRE(frame1.at("recording_day") == 1);
        REQUIRE(frame1.at("recording_date") == 20260917);

        const json& data_block0 = frame0.at("content").at("data_blocks").at(0);
        const json& data_block1 = frame1.at("content").at("data_blocks").at(0);

        REQUIRE(data_block0.at("category") == 247);
        REQUIRE(approximatelyEqual(data_block0.at("recording_time"), 86398.5, 1e-6));
        REQUIRE(data_block0.at("recording_day") == 0);
        REQUIRE(data_block0.at("recording_date") == 20260916);

        REQUIRE(approximatelyEqual(data_block1.at("recording_time"), 0.5, 1e-6));
        REQUIRE(data_block1.at("recording_day") == 1);
        REQUIRE(data_block1.at("recording_date") == 20260917);

        REQUIRE(data_block0.at("content").at("records").at(0).at("010").at("SIC") == 1);
    }

    SECTION("flat: per-record side columns")
    {
        json cat247_columns;
        size_t sum_records = 0;

        jasterix.decodeFile(path, "rff",
            [&](std::unique_ptr<json> json_data, size_t, size_t, size_t num_records, size_t num_errors)
            {
                REQUIRE(num_errors == 0);
                sum_records += num_records;

                REQUIRE(json_data->contains("247"));
                cat247_columns = json_data->at("247");
            },
            true);

        REQUIRE(sum_records == 2);

        const json& time_col = cat247_columns.at("recording_time");
        const json& day_col = cat247_columns.at("recording_day");
        const json& date_col = cat247_columns.at("recording_date");

        REQUIRE(time_col.size() == 2);
        REQUIRE(approximatelyEqual(time_col.at(0), 86398.5, 1e-6));
        REQUIRE(approximatelyEqual(time_col.at(1), 0.5, 1e-6));
        REQUIRE(day_col.at(0) == 0);
        REQUIRE(day_col.at(1) == 1);
        REQUIRE(date_col.at(0) == 20260916);
        REQUIRE(date_col.at(1) == 20260917);

        REQUIRE(cat247_columns.at("010.SAC").size() == 2);
    }

    std::remove(path.c_str());

    loginf << "rff recording time test: end" << logendl;
}

TEST_CASE("jASTERIX RFF recording time without header start time", "[jASTERIX recording time]")
{
    loginf << "rff recording time no start test: start" << logendl;

    // unreadable header text: times stay relative to the recording start, no date
    std::vector<unsigned char> file_bytes;
    appendRFFHeader(file_bytes, "no time here", "");
    appendRFFFrame(file_bytes, 500, cat247_data_block);
    appendRFFFrame(file_bytes, 86400500, cat247_data_block);  // one day and 0.5 s later

    std::string path = writeTempFile("rff_recording_time_nostart", file_bytes);

    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);
    jasterix.category(247)->setCurrentEdition("1.2");

    SECTION("structured")
    {
        json header;
        json all_frames = json::array();

        jasterix.decodeFile(path, "rff",
            [&](std::unique_ptr<json> json_data, size_t, size_t, size_t, size_t num_errors)
            {
                REQUIRE(num_errors == 0);

                header = *json_data;
                header.erase("frames");

                for (const json& frame : json_data->at("frames"))
                    all_frames.push_back(frame);
            },
            false);

        REQUIRE(!header.contains("recording_start_time"));
        REQUIRE(!header.contains("recording_start_date"));

        REQUIRE(all_frames.size() == 2);
        REQUIRE(approximatelyEqual(all_frames.at(0).at("recording_time"), 0.5, 1e-6));
        REQUIRE(all_frames.at(0).at("recording_day") == 0);
        REQUIRE(!all_frames.at(0).contains("recording_date"));
        REQUIRE(approximatelyEqual(all_frames.at(1).at("recording_time"), 0.5, 1e-6));
        REQUIRE(all_frames.at(1).at("recording_day") == 1);

        const json& data_block1 = all_frames.at(1).at("content").at("data_blocks").at(0);
        REQUIRE(data_block1.at("recording_day") == 1);
        REQUIRE(!data_block1.contains("recording_date"));
    }

    SECTION("flat: date column declared but null")
    {
        json cat247_columns;

        jasterix.decodeFile(path, "rff",
            [&](std::unique_ptr<json> json_data, size_t, size_t, size_t, size_t num_errors)
            {
                REQUIRE(num_errors == 0);
                cat247_columns = json_data->at("247");
            },
            true);

        REQUIRE(cat247_columns.at("recording_day").at(1) == 1);
        REQUIRE(cat247_columns.at("recording_date").size() == 2);
        REQUIRE(cat247_columns.at("recording_date").at(0).is_null());
        REQUIRE(cat247_columns.at("recording_date").at(1).is_null());
    }

    std::remove(path.c_str());

    loginf << "rff recording time no start test: end" << logendl;
}
