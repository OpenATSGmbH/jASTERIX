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

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "json.hpp"

struct archive;
struct archive_entry;

namespace jASTERIX
{
typedef enum
{
    JSON_TEXT = 0,
    JSON_ZIP_TEXT
} JSON_OUTPUT_TYPE;

class JSONFileWriteTask;

class JSONWriter
{
  public:
    JSONWriter(JSON_OUTPUT_TYPE json_output_type, const std::string& json_path);
    ~JSONWriter();

    // will move the data
    void write(std::unique_ptr<nlohmann::json> data);

    void fileWritingDone();

  private:
    JSON_OUTPUT_TYPE json_output_type_;
    std::string json_path_;

    bool json_file_open_{false};
    std::ofstream json_file_;

    bool json_zip_file_open_{false};
    struct archive* json_zip_file_{nullptr};

    size_t rec_num_cnt_{0};
    std::vector<std::unique_ptr<nlohmann::json>> json_data_;
    std::vector<std::string> text_data_;
    //std::vector<std::vector<std::uint8_t>> binary_data_;

    bool file_write_in_progress_{false};

    void writeData();

    void convertJSON2Text();

    void openJsonFile();
    void writeTextToFile();
    //void writeBinaryToFile();
    void closeJsonFile();

    void openJsonZipFile();
    void writeTextToZipFile();
    //void writeBinaryToZipFile();
    void closeJsonZipFile();
};

}  // namespace jASTERIX

