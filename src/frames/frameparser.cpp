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

#include "frameparser.h"

#include <tbb/tbb.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>

#include "asterixparser.h"
#include "itemparserbase.h"
#include "jasterix.h"
#include "logger.h"
#include "string_conv.h"
#include "traced_assert.h"

using namespace std;
using namespace nlohmann;

namespace jASTERIX
{
FrameParser::FrameParser(const json& framing_definition, ASTERIXParser& asterix_parser, bool debug)
    : asterix_parser_(asterix_parser)
{
    if (!framing_definition.contains("name"))
        throw runtime_error("frame parser construction without JSON name definition");

    std::string item_name;
    ItemParserBase* item{nullptr};

    if (framing_definition.contains("file_header_items"))
    {
        if (!framing_definition.at("file_header_items").is_array())
            throw runtime_error("frame parser construction with header items non-array");

        for (const json& data_item_it : framing_definition.at("file_header_items"))
        {
            item_name = data_item_it.at("name");

            if (debug)
                loginf << "frame parser constructing file header item '" << item_name << "'"
                       << logendl;

            item = ItemParserBase::createItemParser(data_item_it, "");
            traced_assert(item);
            file_header_items_.push_back(std::unique_ptr<ItemParserBase>{item});
        }

        has_file_header_items_ = true;
    }

    if (!framing_definition.contains("frame_items"))
        throw runtime_error("frame parser construction without frame items");

    if (!framing_definition.at("frame_items").is_array())
        throw runtime_error("frame parser construction with frame items non-array");

    for (const json& data_item_it : framing_definition.at("frame_items"))
    {
        item_name = data_item_it.at("name");

        if (debug)
            loginf << "frame parser constructing frame item '" << item_name << "'" << logendl;

        item = ItemParserBase::createItemParser(data_item_it, "");
        traced_assert(item);
        frame_items_.push_back(std::unique_ptr<ItemParserBase>{item});
    }

    // computed recording time: start wall time from a file header item plus a per-frame
    // offset item scaled to seconds (RFF). framings with direct items (IOSS) name them
    // recording_time / recording_day instead.
    if (framing_definition.contains("recording_time"))
    {
        const json& def = framing_definition.at("recording_time");

        if (!def.is_object() || !def.contains("start_item") || !def.contains("offset_item"))
            throw runtime_error("frame parser construction with invalid recording_time definition");

        has_computed_recording_time_ = true;
        recording_start_item_ = def.at("start_item");
        recording_offset_item_ = def.at("offset_item");

        if (def.contains("offset_lsb"))
            recording_offset_lsb_ = def.at("offset_lsb");
    }
}

size_t FrameParser::parseHeader(const char* data, size_t index, size_t total_size, json& target,
                                bool debug)
{
    traced_assert(data);
    traced_assert(total_size);
    traced_assert(index < total_size);
    // traced_assert(target != nullptr);

    size_t parsed_bytes{0};

    for (auto& j_item : file_header_items_)
    {
        parsed_bytes +=
            j_item->parseItem(data, index + parsed_bytes, total_size, parsed_bytes, total_size, target, debug);
    }

    if (has_computed_recording_time_)
    {
        recording_start_available_ = false;

        if (target.contains(recording_start_item_) && target.at(recording_start_item_).is_string())
        {
            int year, month, day, hours, minutes, seconds;

            if (parseWallTime(target.at(recording_start_item_).get<std::string>(), year, month,
                              day, hours, minutes, seconds))
            {
                recording_start_available_ = true;
                recording_start_sod_ = hours * 3600.0 + minutes * 60.0 + seconds;
                recording_start_year_ = year;
                recording_start_month_ = month;
                recording_start_day_ = day;

                target["recording_start_time"] = recording_start_sod_;
                target["recording_start_date"] = dateYYYYMMDD(year, month, day, 0);
            }
        }

        if (!recording_start_available_)
            logwrn << "frame parser: file header has no readable recording start time, "
                   << "recording_time is relative to the recording start" << logendl;
    }

    return parsed_bytes;
}

// parsed bytes, num frames, done flag, error flag
std::tuple<size_t, size_t, bool, bool> FrameParser::findFrames(const char* data, size_t index,
                                                         size_t total_size, nlohmann::json* target,
                                                         bool debug)
{
    traced_assert(data);
    traced_assert(total_size);
    traced_assert(index < total_size);
    traced_assert(target);

    if (has_file_header_items_)
        traced_assert(target != nullptr);

    size_t parsed_bytes_sum{0};
    size_t parsed_bytes_frame{0};
    size_t parsed_bytes{0};
    size_t chunk_frames_cnt{0};

    if (debug)
        loginf << "finding frames index " << index << " size " << total_size << " num_frames "
               << frame_chunk_size;

    nlohmann::json& j_frames = (*target)["frames"];

    bool hit_frame_limit{false};
    bool hit_frame_chunk_limit{false};
    bool error_flag{false};

    while (index + parsed_bytes_sum < total_size)
    {
        // parse single frame
        if (debug)
            loginf << "finding frame " << sum_frames_cnt_ << " at index "
                   << index + parsed_bytes_sum << " size " << total_size << logendl;

        if (frame_limit > 0 && sum_frames_cnt_ >= static_cast<unsigned>(frame_limit))
        {
            // hit frame limit
            if (debug)
                loginf << "frame parser hit frame limit at " << sum_frames_cnt_ << ", setting done"
                       << logendl;

            hit_frame_limit = true;
            break;
        }

        if (frame_chunk_size > 0 && chunk_frames_cnt >= static_cast<unsigned>(frame_chunk_size))
        {
            // hit frame chunk limit
            if (debug)
                loginf << "frame parser hit frame chunk limit at " << chunk_frames_cnt << logendl;

            hit_frame_chunk_limit = true;
            break;
        }

        parsed_bytes_frame = 0;
        nlohmann::json& current_frame = j_frames[chunk_frames_cnt];
        for (auto& j_item : frame_items_)
        {
            if (index + parsed_bytes_sum >= total_size)
            {
                logerr << "unexpected quit at index " << index + parsed_bytes_sum << " frame pb "
                       << parsed_bytes_frame << " cnt " << chunk_frames_cnt << logendl;
                error_flag = true; // too long
                break;
            }

            if (debug)
                loginf << "found frame item at index " << index + parsed_bytes_sum << " frame pb "
                       << parsed_bytes_frame << " cnt " << chunk_frames_cnt << logendl;

            assert (index + parsed_bytes_sum < total_size);

            parsed_bytes =
                j_item->parseItem(data, index + parsed_bytes_sum, total_size - parsed_bytes_sum,
                                  parsed_bytes_frame, total_size, current_frame, debug);
            parsed_bytes_frame += parsed_bytes;
            parsed_bytes_sum += parsed_bytes;
        }
        if (has_computed_recording_time_)
            addComputedRecordingTime(current_frame);

        current_frame["cnt"] = chunk_frames_cnt;
        //        loginf << "UGA FP FOUND '" << j_frames[chunk_frames_cnt].dump(4) << "'" <<
        //        logendl;

        ++chunk_frames_cnt;
        ++sum_frames_cnt_;
    }

    bool done_flag = hit_frame_limit ? true : !hit_frame_chunk_limit;
    // done if frame limit hit, if not -> done if frame chunk limit not hit

    return std::make_tuple(parsed_bytes_sum, chunk_frames_cnt, done_flag, error_flag);
}

std::pair<size_t, size_t> FrameParser::decodeFrames(const char* data, size_t total_size, json* target, bool debug)
{
    traced_assert(data);
    traced_assert(target != nullptr);

    //    loginf << "FrameParser: decodeFrames" << logendl;

    std::pair<size_t, size_t> ret{0, 0};
    nlohmann::json& j_frames = target->at("frames");

    if (debug || single_thread || asterix_parser_.flatMode())  // single thread in debug or flat mode
    {
        std::pair<size_t, size_t> tmp{0, 0};

        for (json& frame_it : j_frames)
        {
            tmp = decodeFrame(data, total_size, frame_it, debug);
            ret.first += tmp.first;
            ret.second += tmp.second;
        }
    }
    else
    {
        size_t num_frames = j_frames.size();
        std::vector<std::pair<size_t, size_t>> dec_ret;
        dec_ret.resize(num_frames, {0, 0});

        tbb::parallel_for(size_t(0), num_frames, [&](size_t cnt) {
            dec_ret.at(cnt) = decodeFrame(data, total_size, j_frames.at(cnt), debug);
        });

        for (auto num_record_it : dec_ret)
        {
            ret.first += num_record_it.first;
            ret.second += num_record_it.second;
        }
    }

    if (debug)
        loginf << "frames decoded, num frames " << ret.first << " num errors " << ret.second
               << logendl;

    return ret;
}

bool FrameParser::hasFileHeaderItems() const { return has_file_header_items_; }

std::vector<std::string> FrameParser::recordingKeys(const json& framing_definition)
{
    const std::vector<std::string> all_keys{"recording_time", "recording_day", "recording_date"};

    if (framing_definition.contains("recording_time")
        && framing_definition.at("recording_time").is_object())
        return all_keys;

    std::vector<std::string> keys;

    if (framing_definition.contains("frame_items") && framing_definition.at("frame_items").is_array())
    {
        for (const json& item : framing_definition.at("frame_items"))
        {
            if (!item.contains("name"))
                continue;

            for (const std::string& key : all_keys)
                if (item.at("name") == key)
                    keys.push_back(key);
        }
    }

    return keys;
}

bool FrameParser::parseWallTime(const std::string& text, int& year, int& month, int& day,
                                int& hours, int& minutes, int& seconds)
{
    // the position checks below need at least 19 characters
    std::string buffer = text;
    if (buffer.size() < 19)
        buffer.resize(19, '\0');

    // fixed-width decimal field, digits only
    auto field = [&buffer](size_t pos, size_t len, int& value) -> bool {
        std::string digits = buffer.substr(pos, len);

        if (digits.empty()
            || !std::all_of(digits.begin(), digits.end(),
                            [](unsigned char c) { return std::isdigit(c); }))
            return false;

        value = std::stoi(digits);
        return true;
    };

    // two-digit years: 70..99 are 19xx, 00..69 are 20xx, as in the SDL reference reader
    auto century = [](int& y) { y += (y >= 70) ? 1900 : 2000; };

    if (buffer[3] == '/' && buffer[6] == '/' && buffer[9] == ' ' && buffer[12] == ':'
        && buffer[15] == ':')
    {
        // US style " MM/DD/YY HH:MM:SS"
        if (!field(1, 2, month) || !field(4, 2, day) || !field(7, 2, year) || !field(10, 2, hours)
            || !field(13, 2, minutes) || !field(16, 2, seconds))
            return false;

        century(year);
    }
    else if (buffer[2] == '/' && buffer[5] == '/' && buffer[8] == ' ' && buffer[11] == ':'
             && buffer[14] == ':')
    {
        // German style "DD/MM/YY HH:MM:SS"
        if (!field(0, 2, day) || !field(3, 2, month) || !field(6, 2, year) || !field(9, 2, hours)
            || !field(12, 2, minutes) || !field(15, 2, seconds))
            return false;

        if (month > 12 && day <= 12)
            std::swap(month, day);

        century(year);
    }
    else if (buffer[4] == '-' && buffer[7] == '-' && buffer[10] == ' ' && buffer[13] == ':'
             && buffer[16] == ':')
    {
        // "YYYY-MM-DD HH:MM:SS"
        if (!field(0, 4, year) || !field(5, 2, month) || !field(8, 2, day) || !field(11, 2, hours)
            || !field(14, 2, minutes) || !field(17, 2, seconds))
            return false;
    }
    else
        return false;

    return month >= 1 && month <= 12 && day >= 1 && day <= 31 && hours < 24 && minutes < 60
           && seconds < 60;
}

unsigned int FrameParser::dateYYYYMMDD(int year, int month, int day, int add_days)
{
    // timegm normalizes an overflowing day of month into the following months
    struct tm tm_date {};
    tm_date.tm_year = year - 1900;
    tm_date.tm_mon = month - 1;
    tm_date.tm_mday = day + add_days;

    time_t secs = timegm(&tm_date);

    struct tm tm_utc;
    gmtime_r(&secs, &tm_utc);

    return static_cast<unsigned int>((tm_utc.tm_year + 1900) * 10000 + (tm_utc.tm_mon + 1) * 100
                                     + tm_utc.tm_mday);
}

void FrameParser::addComputedRecordingTime(json& frame)
{
    if (!frame.contains(recording_offset_item_) || !frame.at(recording_offset_item_).is_number())
        return;

    constexpr double seconds_per_day = 86400.0;

    double offset = frame.at(recording_offset_item_).get<double>() * recording_offset_lsb_;
    double total = (recording_start_available_ ? recording_start_sod_ : 0.0) + offset;

    unsigned int days = static_cast<unsigned int>(std::floor(total / seconds_per_day));

    frame["recording_time"] = total - days * seconds_per_day;
    frame["recording_day"] = days;

    if (recording_start_available_)
        frame["recording_date"] = dateYYYYMMDD(recording_start_year_, recording_start_month_,
                                               recording_start_day_, static_cast<int>(days));
}

std::pair<size_t, size_t> FrameParser::decodeFrame(const char* data, size_t total_size, json& json_frame, bool debug)
{
    if (!json_frame.contains("content"))
        throw runtime_error("frame parser scoped frames does not contain correct content");

    //    {
    //        "cnt": 0,
    //        "content": {
    //            "index": 134,
    //            "length": 56
    //        },
    //        "frame_length": 56,
    //        "frame_relative_time_ms": 2117
    //    }

    //    loginf << "UGA FP decode '" << json_frame.dump(4) << "'" << logendl;

    json& frame_content = json_frame.at("content");  // todo what if multiple data blocks?

    size_t index = frame_content.at("index");
    size_t size = frame_content.at("length");

    if (debug)
    {
        loginf << "FrameParser: decodeFrame: index " << index << " length "
               << size << " data '"
               << binary2hex_bounded((const unsigned char*)data, index, size, total_size)
               << "'" << logendl;
    }

    std::tuple<size_t, size_t, bool, bool> db_ret =
        asterix_parser_.findDataBlocks(data, index, size, total_size, &frame_content, debug);

    // parsed_bytes += std::get<0>(ret);
    // size_t num_data_blocks = std::get<1>(ret);

    bool error = std::get<2>(db_ret);  // error flag

    traced_assert(std::get<3>(db_ret));  // done flag

    if (!frame_content.contains("data_blocks"))
        throw runtime_error("frame parser scoped frames do not contain data blocks");

    if (!frame_content.at("data_blocks").is_array())
        throw runtime_error("frame parser scoped frames data blocks are non-array");

    // recording time of the frame applies to every data block in it. set before the records
    // are decoded, so flat mode can copy the values per record.
    for (const char* key : {"recording_time", "recording_day", "recording_date"})
    {
        if (!json_frame.contains(key))
            continue;

        for (json& data_block : frame_content.at("data_blocks"))
            data_block[key] = json_frame.at(key);
    }

    std::pair<size_t, size_t> ret{0, 0};
    std::pair<size_t, size_t> dec_ret{0, 0};

    if (error)  // add data block errors
        ret.second += 1;

    for (json& data_block : frame_content.at("data_blocks"))
    {
        dec_ret = asterix_parser_.decodeDataBlock(data, total_size, data_block, debug);
        ret.first += dec_ret.first;
        ret.second += dec_ret.second;
    }

    // loginf << "FP UGA '" << json_frame.dump(4) << "'" << logendl;

    return ret;
}

}  // namespace jASTERIX
