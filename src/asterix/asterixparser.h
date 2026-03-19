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

#include <tuple>

#include "category.h"
#include "edition.h"
#include "jasterix/global.h"
#include "json.hpp"
#include "mapping.h"

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
    void setFlatData(std::map<unsigned int, nlohmann::json>* data);
    bool flatMode() const { return flat_record_indices_ != nullptr; }

  private:
    std::string data_block_name_;
    std::vector<std::unique_ptr<ItemParserBase>> data_block_items_;
    std::map<unsigned int, std::shared_ptr<Record>> records_;
    std::map<unsigned int, std::shared_ptr<Mapping>> mappings_;

    std::map<unsigned int, size_t>* flat_record_indices_{nullptr};
    std::map<unsigned int, nlohmann::json*>* flat_hash_columns_{nullptr};
    std::map<unsigned int, nlohmann::json>* flat_data_{nullptr};

#if USE_OPENSSL
    void calculateARTASMD5Hash(const char* data, size_t length, nlohmann::json& target);
#endif
};

}  // namespace jASTERIX

