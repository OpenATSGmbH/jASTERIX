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

#include <jasterix/category.h>
#include <jasterix/edition.h>
#include <jasterix/frameparser.h>
#include <jasterix/global.h>
#include <jasterix/mapping.h>

#include <boost/iostreams/device/mapped_file.hpp>
#include <deque>
#include <map>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>

#include "json.hpp"

namespace jASTERIX
{
extern int print_dump_indent;
extern int frame_limit;
extern int frame_chunk_size;
extern int data_block_limit;
extern int data_block_chunk_size;
extern int data_write_size;

extern bool single_thread;

#if USE_OPENSSL
extern bool add_artas_md5_hash;
#endif

extern bool add_record_data;

class DataBlockFinderTask;
class FrameParserTask;

class jASTERIX
{
  public:
    jASTERIX(const std::string& definition_path, bool print, bool debug,
             bool debug_exclude_framing);
    virtual ~jASTERIX();

    bool hasCategory(unsigned int cat);
    bool decodeCategory(unsigned int cat);
    void setDecodeCategory(unsigned int cat, bool decode);
    void decodeNoCategories();

    std::shared_ptr<Category> category(unsigned int cat);
    const std::map<unsigned int, std::shared_ptr<Category>>& categories()
    {
        return category_definitions_;
    }

    std::unique_ptr<nlohmann::json> analyzeFile(const std::string& filename, const std::string& framing_str,
                                                unsigned int record_limit=0);
    std::unique_ptr<nlohmann::json> analyzeFile(const std::string& filename,
                                                unsigned int record_limit=0);

    std::string analyzeFileCSV(const std::string& filename, const std::string& framing_str,
                               unsigned int record_limit=0);
    std::string analyzeFileCSV(const std::string& filename,
                               unsigned int record_limit=0);

    std::unique_ptr<nlohmann::json> analyzeData(const char* data, unsigned int total_size,
                                                unsigned int record_limit=0);

    std::string analyzeDataCSV(const char* data, unsigned int total_size,
                                                unsigned int record_limit=0);

    void decodeFile(const std::string& filename, const std::string& framing_str,
                    std::function<void(std::unique_ptr<nlohmann::json>, size_t, size_t, size_t)>
                        data_callback = nullptr);
    // callback gets moved chunk, accumulated number of frames, number of records, number of errors
    void decodeFile(const std::string& filename,
                    std::function<void(std::unique_ptr<nlohmann::json>, size_t, size_t, size_t)>
                        data_callback = nullptr);
    void stopDecoding();

    void decodeData(const char* data, unsigned int total_size,
                    std::function<void(std::unique_ptr<nlohmann::json>, size_t, size_t, size_t)>
                    data_callback = nullptr,
                    bool abortable = true);

    size_t numFrames() const;
    size_t numRecords() const;
    size_t numErrors() const;

    void addDataBlockChunk(std::unique_ptr<nlohmann::json> data_block_chunk, bool error, bool done);
    void addDataChunk(std::unique_ptr<nlohmann::json> data_chunk, bool done);

    const std::vector<std::string>& framings() { return framings_; }

    const std::string& dataBlockDefinitionPath() const;
    const std::string& categoriesDefinitionPath() const;
    const std::string& framingsFolderPath() const;

    void setDebug(bool debug);

  private:
    std::string definition_path_;
    bool print_{false};
    bool debug_{false};
    bool debug_exclude_framing_{false};

    std::string framing_path_;
    std::vector<std::string> framings_;

    std::string data_block_definition_path_;
    nlohmann::json data_block_definition_;

    std::string categories_definition_path_;
    nlohmann::json categories_definition_;

    std::map<unsigned int, std::shared_ptr<Category>> category_definitions_;

    boost::iostreams::mapped_file_source file_;

    std::deque<std::unique_ptr<nlohmann::json>> data_block_chunks_;
    std::mutex data_block_chunks_mutex_;
    std::condition_variable data_block_chunks_cv_;
    bool data_block_processing_done_{false};

    std::deque<std::unique_ptr<nlohmann::json>> data_chunks_;
    std::mutex data_chunks_mutex_;
    std::condition_variable data_chunks_cv_;
    bool data_processing_done_{false};

    size_t num_frames_{0};
    size_t num_records_{0};
    size_t num_errors_{0};

    std::atomic<bool> stop_decoding_{false};

    // sac/sic -> cat -> count
    //std::map<std::string, std::map<std::string, unsigned int>> sensor_counts_;

    // sac/sic -> cat -> key -> count/min/max
    std::map<std::string, std::map<std::string, std::map<std::string, nlohmann::json>>> data_item_analysis_;

    size_t openFile (const std::string& filename); // returns file size
    nlohmann::json loadFramingDefinition(const std::string& framing_str);
    void analyzeChunk(const std::unique_ptr<nlohmann::json>& data_chunk, bool framing);
    void analyzeRecord(unsigned int category, const nlohmann::json& record);

    void addJSONAnalysis(const std::string& sensor_id, const std::string& cat_str,
                         const std::string& prefix, const nlohmann::json& item);

    void clearDataChunks();
    void clearDataBlockChunks();

    std::string toCSV (const std::map<std::string, std::map<std::string, std::map<std::string, nlohmann::json>>>& data_item_analysis);

    void forceStopTask (DataBlockFinderTask& task);
    void forceStopTask (FrameParserTask& task);
};
}  // namespace jASTERIX


