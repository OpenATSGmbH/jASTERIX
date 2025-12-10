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

  private:
    ASTERIXParser& asterix_parser_;

    bool has_file_header_items_{false};
    std::vector<std::unique_ptr<ItemParserBase>> file_header_items_;
    std::vector<std::unique_ptr<ItemParserBase>> frame_items_;

    size_t sum_frames_cnt_{0};

    // returns number of records, num errors
    std::pair<size_t, size_t> decodeFrame(const char* data, size_t total_size, nlohmann::json& json_frame, bool debug);
};

}  // namespace jASTERIX


