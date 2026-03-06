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

#include <map>
#include <memory>

#include "json.hpp"

struct RecordInfo
{
    RecordInfo(unsigned int category, unsigned int index, unsigned int length,
               const std::string& record_data)
        : category_(category), index_(index), length_(length), record_data_(record_data)
    {
    }
    unsigned int category_;
    unsigned int index_;       // data block index
    unsigned int length_;      // data block length
    std::string record_data_;  // hex_dump, optional
};

class HashChecker
{
  public:
    HashChecker(bool framing_used);

    void process(std::unique_ptr<nlohmann::json> data);
    void printCollisions();

  private:
    bool framing_used_{false};

    std::map<std::string, std::vector<RecordInfo>> hash_map_;  // hash -> [RecordInfo]

    // data block index, length
    void processRecord(unsigned int category, nlohmann::json& record);
};

extern std::string check_artas_md5_hash;
extern std::vector<int> check_artas_md5_categories;
extern std::unique_ptr<HashChecker> hash_checker;

extern void check_callback(std::unique_ptr<nlohmann::json> data_chunk, size_t total_num_bytes,
                           size_t num_frames, size_t num_records, size_t num_errors);


