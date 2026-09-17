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

#include <atomic>
#include <tuple>

#include "category.h"
#include "edition.h"
#include "jasterix/global.h"
#include "json.hpp"

namespace jASTERIX
{
class ItemParserBase;

class ASTERIXParser
{
  public:
    ASTERIXParser(const nlohmann::json& data_block_definition,
                  std::map<unsigned int, std::shared_ptr<Category>>& category_definitions,
                  bool debug);

    // parsed bytes, num data blocks, error flag, done flag
    std::tuple<size_t, size_t, bool, bool> findDataBlocks(const char* data, size_t index,
                                                          size_t length, size_t total_size,
                                                          nlohmann::json* target, bool debug);

    // num recored, num errors
    std::pair<size_t, size_t> decodeDataBlocks(const char* data, size_t total_size, nlohmann::json& data_blocks,
                                               bool debug);
    std::pair<size_t, size_t> decodeDataBlock(const char* data, size_t total_size, nlohmann::json& data_block,
                                              bool debug);
    //    size_t decodeDataBlock (const char* data, unsigned int cat, size_t index, size_t size,
    //    nlohmann::json& target,
    //                            bool debug);

    void setFlatRecordIndices(std::map<unsigned int, size_t>* indices);
    void setFlatHashColumns(std::map<unsigned int, nlohmann::json*>* columns);
    void setFlatRecordDataColumns(std::map<unsigned int, nlohmann::json*>* columns);
    void setFlatData(std::map<unsigned int, nlohmann::json>* data);
    // data block keys copied into per-record side columns (cat -> key -> column), e.g.
    // recording_time / recording_day / recording_date, set on the data block object by the
    // frame parser or the PCAP time stamping before the records are decoded
    void setFlatDataBlockKeyColumns(
        std::map<unsigned int, std::map<std::string, nlohmann::json*>>* columns);
    bool flatMode() const { return flat_record_indices_ != nullptr; }

    // records whose REF/SPF content did not match the definition and was kept as raw
    // data (see Record::parseItem fallback); cumulative over this parser's lifetime
    size_t numREFErrors() const { return num_ref_errors_; }
    size_t numSPFErrors() const { return num_spf_errors_; }

  private:
    std::string data_block_name_;
    std::vector<std::unique_ptr<ItemParserBase>> data_block_items_;
    std::map<unsigned int, std::shared_ptr<Record>> records_;

    // drops the cells a rejected record already wrote into the flat columns
    void dropFlatRecord(unsigned int cat);

    std::map<unsigned int, size_t>* flat_record_indices_{nullptr};
    std::map<unsigned int, nlohmann::json*>* flat_hash_columns_{nullptr};
    std::map<unsigned int, nlohmann::json*>* flat_record_data_columns_{nullptr};
    std::map<unsigned int, nlohmann::json>* flat_data_{nullptr};
    std::map<unsigned int, std::map<std::string, nlohmann::json*>>* flat_data_block_key_columns_{nullptr};

    // atomic: data blocks of one chunk are decoded in parallel (TBB) sharing this parser
    std::atomic<size_t> num_ref_errors_{0};
    std::atomic<size_t> num_spf_errors_{0};

    // Last full I002/030 Time of Day per data source (key: "SAC/SIC"),
    // always in [0, 86400). Used in flat mode as reference to reconstruct
    // the full time from the CAT001 truncated Time of Day (I001/141), as
    // recommended in CAT001 Part 2a section 5.3.2.7.
    std::map<std::string, double> cat002_last_tod_;

#if USE_OPENSSL
    void calculateARTASMD5Hash(const char* data, size_t length, nlohmann::json& target);
#endif
};

}  // namespace jASTERIX

