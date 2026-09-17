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
// records to process in decode and analyze, -1 for no limit. processing stops at the end of
// the chunk (frame_chunk_size / data_block_chunk_size) in which the limit is reached, so
// slightly more records than the limit are delivered.
extern int record_limit;

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

    // PCAP variants: extract the ASTERIX payload from a PCAP capture (libpcap, raw/netto,
    // no framing) and analyze it per detected network stream (signature). The returned JSON
    // is keyed by signature string, each value being the analyzeData result for that stream.
    std::unique_ptr<nlohmann::json> analyzePCAPFile(const std::string& filename,
                                                    unsigned int record_limit=0);
    std::string analyzePCAPFileCSV(const std::string& filename,
                                   unsigned int record_limit=0);

    // Callback signature for decodeFile/decodeData:
    //   data          - decoded JSON chunk (frames or data_blocks, or flat columnar data)
    //   total_num_bytes - cumulative number of bytes decoded so far (for progress tracking)
    //   num_frames      - number of frames in this chunk (0 when decoding without framing)
    //   num_records     - number of records decoded in this chunk
    //   num_errors      - number of decode errors in this chunk
    using decode_callback_t = std::function<void(std::unique_ptr<nlohmann::json> data,
                                                 size_t total_num_bytes,
                                                 size_t num_frames,
                                                 size_t num_records,
                                                 size_t num_errors)>;

    // do_flat=false: callback receives {"frames":[...]} or {"data_blocks":[...]} with nested per-record objects.
    // do_flat=true:  callback receives {"<cat>": {"<item>.<field>": [val_per_record, ...], ...}, ...} per chunk.
    //                Forces single-thread mode. Column arrays are moved out each chunk; fresh arrays created for next.
    void decodeFile(const std::string& filename, const std::string& framing_str,
                    decode_callback_t data_callback = nullptr,
                    bool do_flat = false);
    void decodeFile(const std::string& filename,
                    decode_callback_t data_callback = nullptr,
                    bool do_flat = false);
    void stopDecoding();

    void decodeData(const char* data, unsigned int total_size,
                    decode_callback_t data_callback = nullptr,
                    bool abortable = true,
                    bool do_flat = false);

    // Decode an ASTERIX PCAP capture (libpcap). Extracts the payload of all network streams
    // in capture order (raw/netto, no framing) and decodes it in chunks via decodeData.
    void decodePCAPFile(const std::string& filename,
                        decode_callback_t data_callback = nullptr,
                        bool do_flat = false);

    // Encode a single record for a given category into a data block (CAT + LEN + record).
    std::vector<char> encodeRecord(unsigned int category,
                                   const nlohmann::json& record_json,
                                   bool debug = false);

    // Encode multiple records into a single data block (CAT + LEN + records).
    std::vector<char> encodeDataBlock(unsigned int category,
                                      const std::vector<nlohmann::json>& records,
                                      bool debug = false);

    size_t numFrames() const;
    size_t numRecords() const;
    size_t numErrors() const;

    // records whose REF/SPF content did not match the selected REF/SPF definition and
    // was kept as raw hex data (with a ref_error/spf_error flag in the record);
    // cumulative over the last decode/analyze call
    size_t numREFErrors() const;
    size_t numSPFErrors() const;

    void addDataBlockChunk(std::unique_ptr<nlohmann::json> data_block_chunk, size_t bytes_read,
                           bool error, bool done);
    void addDataChunk(std::unique_ptr<nlohmann::json> data_chunk, size_t bytes_read, bool done);

    // Called from a producer task when an exception aborts production. Marks the
    // corresponding queue as done and wakes any consumer parked on the cv, so
    // the consumer breaks out of its wait loop instead of deadlocking.
    void notifyDataChunksError();
    void notifyDataBlockChunksError();

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

    std::deque<std::pair<std::unique_ptr<nlohmann::json>, size_t>> data_block_chunks_;  // {chunk, bytes_read}
    std::mutex data_block_chunks_mutex_;
    std::condition_variable data_block_chunks_cv_;
    bool data_block_processing_done_{false};

    std::deque<std::pair<std::unique_ptr<nlohmann::json>, size_t>> data_chunks_;  // {chunk, bytes_read}
    std::mutex data_chunks_mutex_;
    std::condition_variable data_chunks_cv_;
    bool data_processing_done_{false};

    size_t num_frames_{0};
    size_t num_records_{0};
    size_t num_errors_{0};
    size_t num_ref_errors_{0};
    size_t num_spf_errors_{0};

    std::atomic<bool> stop_decoding_{false};

    // num_records_ at the start of the current decode or analyze call. record_limit counts
    // the records of that call, although num_records_ accumulates over the instance lifetime.
    size_t record_limit_base_{0};
    bool recordLimitReached() const;

    // during PCAP decode: packet payload offset -> capture time (seconds since epoch, UTC)
    // for the current chunk, sorted by offset. nullptr when not decoding a PCAP.
    const std::vector<std::pair<std::size_t, double>>* pcap_packet_times_{nullptr};

    // Flat/columnar mode state
    std::map<unsigned int, nlohmann::json> flat_data_;       // cat -> {leaf_name -> json::array}
    std::map<unsigned int, size_t> flat_record_indices_;     // cat -> current record index
    std::map<unsigned int, nlohmann::json*> flat_hash_columns_; // cat -> pointer to artas_md5 array
    std::map<unsigned int, nlohmann::json*> flat_record_data_columns_; // cat -> pointer to record_data array

    // data block keys (recording_time, recording_day, recording_date) which become per-record
    // side columns. set per decode path from the framing definition or the PCAP source.
    std::vector<std::string> flat_data_block_keys_;
    std::map<unsigned int, std::map<std::string, nlohmann::json*>> flat_data_block_key_columns_; // cat -> key -> column

    // analysis statistics, filled from the flat columns per chunk: sensor "SAC/SIC" ->
    // category -> record count and per item path (flat column name) count, min and max
    struct ItemAnalysis
    {
        size_t count{0};
        bool has_bounds{false};  // scalar values only, arrays are counted
        nlohmann::json min;
        nlohmann::json max;
    };
    struct CategoryAnalysis
    {
        size_t count{0};  // records
        std::map<std::string, ItemAnalysis> items;
    };
    std::map<std::string, std::map<std::string, CategoryAnalysis>> analysis_;

    // set while an analyze call runs the flat decode: the chunk loops feed analyzeFlatChunk
    // instead of print and callback, and stop after the first chunk with decode errors
    bool analysis_mode_{false};
    size_t analysis_errors_base_{0};
    // record limit of the running analyze call, 0 for the global record_limit
    unsigned int call_record_limit_{0};

    // cat -> {num data blocks, num bytes} of data blocks skipped during analysis
    // because the category could not be decoded (no definition or decoding disabled)
    std::map<unsigned int, std::pair<size_t, size_t>> skipped_category_counts_;

    size_t openFile (const std::string& filename); // returns file size
    nlohmann::json loadFramingDefinition(const std::string& framing_str);
    // runs decode (a flat decode call) in analysis mode and builds the result
    std::unique_ptr<nlohmann::json> runAnalysis(const std::function<void()>& decode,
                                                unsigned int record_limit);
    // statistics of one flat chunk, chunk holds the frames or data blocks it came from
    void analyzeFlatChunk(const nlohmann::json& flat_chunk, const nlohmann::json& chunk,
                          bool framing);
    void addAnalysisResult(nlohmann::json& analysis_result);
    void countSkippedDataBlock(const nlohmann::json& data_block);
    void addSkippedCategoriesAnalysis(nlohmann::json& analysis_result);
    // record limit reached, or decode errors while analyzing
    bool stopAfterChunk() const;

    void clearDataChunks();
    void clearDataBlockChunks();
    // clears leftover chunks and done flags of an earlier decode or analyze call
    void resetChunkState();

    std::string toCSV(const nlohmann::json& analysis_result);

    void setupFlatColumns();
    // leaves columnar mode: a flat call sets column targets into flat_data_ on the parser
    // tree, a structured call on the same instance must write into the records again
    void clearFlatColumns();
    std::unique_ptr<nlohmann::json> moveFlatData();

    void forceStopTask (DataBlockFinderTask& task);
    void forceStopTask (FrameParserTask& task);

    // stamps each data block in the array with "pcap_time"/"pcap_time_epoch" and the
    // derived "recording_time" (seconds since UTC midnight) / "recording_date" (YYYYMMDD)
    // using pcap_packet_times_ and the data block's content index.
    void stampPCAPTimes(nlohmann::json& data_blocks, bool with_time_string);
};
}  // namespace jASTERIX


