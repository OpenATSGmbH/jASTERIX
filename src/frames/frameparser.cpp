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

    return parsed_bytes;
}

std::tuple<size_t, size_t, bool> FrameParser::findFrames(const char* data, size_t index,
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
        for (auto& j_item : frame_items_)
        {
            if (debug)
                loginf << "found frame item at index " << index + parsed_bytes_sum << " frame pb "
                       << parsed_bytes_frame << " cnt " << chunk_frames_cnt << logendl;

            parsed_bytes =
                j_item->parseItem(data, index + parsed_bytes_sum, total_size - parsed_bytes_sum,
                                  parsed_bytes_frame, total_size, j_frames[chunk_frames_cnt], debug);
            j_frames[chunk_frames_cnt]["cnt"] = chunk_frames_cnt;
            parsed_bytes_frame += parsed_bytes;
            parsed_bytes_sum += parsed_bytes;
        }
        //        loginf << "UGA FP FOUND '" << j_frames[chunk_frames_cnt].dump(4) << "'" <<
        //        logendl;

        ++chunk_frames_cnt;
        ++sum_frames_cnt_;
    }

    return std::make_tuple(parsed_bytes_sum, chunk_frames_cnt,
                           hit_frame_limit ? true : !hit_frame_chunk_limit);
    // done if frame limit hit, if not -> done if frame chunk limit not hit
}

std::pair<size_t, size_t> FrameParser::decodeFrames(const char* data, size_t total_size, json* target, bool debug)
{
    traced_assert(data);
    traced_assert(target != nullptr);

    //    loginf << "FrameParser: decodeFrames" << logendl;

    std::pair<size_t, size_t> ret{0, 0};
    nlohmann::json& j_frames = target->at("frames");

    if (debug || single_thread)  // switch to single thread in debug
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
    //        "frame_relative_time_ms": 1158152192
    //    }

    //    loginf << "UGA FP decode '" << json_frame.dump(4) << "'" << logendl;

    json& frame_content = json_frame.at("content");  // todo what if multiple data blocks?

    size_t index = frame_content.at("index");
    size_t size = frame_content.at("length");

    if (debug)
    {
        loginf << "FrameParser: decodeFrame: index " << index << " length "
               << size << " data '"
               << binary2hex((const unsigned char*)&data[index], size)
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
