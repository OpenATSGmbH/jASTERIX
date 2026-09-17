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

#pragma once

#include "json.hpp"

namespace jASTERIX
{
class ASTERIXParser;
class ItemParserBase;

class FrameParser
{
  public:
    FrameParser(const nlohmann::json& framing_definition, ASTERIXParser& asterix_parser,
                bool debug);

    // return number of parsed bytes
    size_t parseHeader(const char* data, size_t index, size_t total_size, nlohmann::json& target,
                       bool debug);

    // parsed bytes, num frames, done flag, error flag
    std::tuple<size_t, size_t, bool, bool> findFrames(const char* data, size_t index, size_t total_size,
                                                nlohmann::json* target, bool debug);

    // num records, num errors
    std::pair<size_t, size_t> decodeFrames(const char* data, size_t total_size, nlohmann::json* target, bool debug);

    bool hasFileHeaderItems() const;

    // recording time keys (recording_time, recording_day, recording_date) a framing writes
    // into its frames and data blocks: frame items with these names, or the keys of the
    // computed "recording_time" definition (file header start plus per-frame offset)
    static std::vector<std::string> recordingKeys(const nlohmann::json& framing_definition);

    // parses a recording wall time text as written into RFF file headers. accepted styles,
    // detected by separator positions: " MM/DD/YY HH:MM:SS" (US, leading space),
    // "DD/MM/YY HH:MM:SS" (German), "YYYY-MM-DD HH:MM:SS". two-digit years >= 70 are 19xx.
    // returns false when no style matches.
    static bool parseWallTime(const std::string& text, int& year, int& month, int& day,
                              int& hours, int& minutes, int& seconds);

    // UTC date as YYYYMMDD of the given date plus a number of days
    static unsigned int dateYYYYMMDD(int year, int month, int day, int add_days);

  private:
    ASTERIXParser& asterix_parser_;

    bool has_file_header_items_{false};
    std::vector<std::unique_ptr<ItemParserBase>> file_header_items_;
    std::vector<std::unique_ptr<ItemParserBase>> frame_items_;

    size_t sum_frames_cnt_{0};

    // computed recording time (RFF): start wall time from the file header plus a per-frame
    // offset item, scaled to seconds by offset_lsb
    bool has_computed_recording_time_{false};
    std::string recording_start_item_;
    std::string recording_offset_item_;
    double recording_offset_lsb_{1.0};

    bool recording_start_available_{false};
    double recording_start_sod_{0.0};  // seconds since midnight at recording start
    int recording_start_year_{0};
    int recording_start_month_{0};
    int recording_start_day_{0};

    // writes recording_time / recording_day / recording_date into a frame from the computed
    // definition
    void addComputedRecordingTime(nlohmann::json& frame);

    // returns number of records, num errors
    std::pair<size_t, size_t> decodeFrame(const char* data, size_t total_size, nlohmann::json& json_frame, bool debug);
};

}  // namespace jASTERIX
