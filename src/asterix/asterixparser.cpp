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

#include "asterixparser.h"

#include <tbb/tbb.h>

#include <iomanip>
#include <iostream>

#include "itemparserbase.h"
#include "jasterix.h"
#include "logger.h"
#include "record.h"
#include "string_conv.h"
#include "traced_assert.h"

#if USE_OPENSSL
#include <openssl/md5.h>
#endif

using namespace std;
using namespace nlohmann;

namespace jASTERIX
{
ASTERIXParser::ASTERIXParser(
    const nlohmann::json& data_block_definition,
    std::map<unsigned int, std::shared_ptr<Category>>& category_definitions, bool debug)
{
    // data block

    if (!data_block_definition.contains("name"))
        throw runtime_error("data block construction without JSON name definition");

    data_block_name_ = data_block_definition.at("name");

    if (!data_block_definition.contains("items"))
        throw runtime_error("data block construction without header items");

    if (!data_block_definition.at("items").is_array())
        throw runtime_error("data block construction with items non-array");

    std::string item_name;
    ItemParserBase* item{nullptr};

    for (const json& data_item_it : data_block_definition.at("items"))
    {
        item_name = data_item_it.at("name");

        if (debug)
            loginf << "asterix parser constructing data block item '" << item_name << "'"
                   << logendl;

        item = ItemParserBase::createItemParser(data_item_it, "");
        traced_assert(item);
        data_block_items_.push_back(std::unique_ptr<ItemParserBase>{item});
    }

    // asterix record definitions

    for (auto& cat_it : category_definitions)
    {
        if (debug)
            loginf << "asterix parser constructing cat '" << setfill('0') << setw(3) << cat_it.first
                   << "'" << logendl;

        if (!cat_it.second->decode())  // if not decode, do not store record
        {
            if (debug)
                loginf << "asterix parser not decoding cat '" << setfill('0') << setw(3)
                       << cat_it.first << "'" << logendl;
            continue;
        }

        if (debug)
            loginf << "asterix parser decoding cat '" << setfill('0') << setw(3) << cat_it.first
                   << "'" << logendl;

        records_.insert(std::pair<unsigned int, std::shared_ptr<Record>>(
            cat_it.first, std::shared_ptr<Record>{cat_it.second->getCurrentEdition()->record()}));

        if (cat_it.second->hasCurrentREFEdition())
            records_.at(cat_it.first)
                ->setRef(cat_it.second->getCurrentREFEdition()->reservedExpansionField());

        if (cat_it.second->hasCurrentSPFEdition())
            records_.at(cat_it.first)
                ->setSpf(cat_it.second->getCurrentSPFEdition()->specialPurposeField());

        if (cat_it.second->hasCurrentMapping())
        {
            mappings_.insert(std::pair<unsigned int, std::shared_ptr<Mapping>>(
                cat_it.first, std::shared_ptr<Mapping>{cat_it.second->getCurrentMapping()}));
        }
    }
}

std::tuple<size_t, size_t, bool, bool> ASTERIXParser::findDataBlocks(
        const char* data, size_t index, size_t length, size_t total_size,
        nlohmann::json* target, bool debug)
{
    if (debug)
        loginf << "asterix parser finding data blocks at index " << index << " length " << length
               << logendl;

    traced_assert(target);

    size_t parsed_bytes{0};
    size_t parsed_data_block_bytes{0};
    size_t parsed_bytes_sum{0};
    size_t num_blocks{0};
    bool error{false};

    //    loginf << "UGA AP index " << index << " length " << length << logendl;

    bool hit_data_block_limit{false};
    bool hit_data_block_chunk_limit{false};
    size_t current_index{index};

    try
    {
        while (parsed_bytes_sum < length)
        {
            //        loginf << "UGA AP1 parsed sum " << parsed_bytes_sum << " length " << length <<
            //        " block " << num_blocks
            //        << logendl;

            if (data_block_limit > 0 && num_blocks >= static_cast<unsigned>(data_block_limit))
            {
                // hit data block limit
                if (debug)
                    loginf << "frame parser hit data block limit at " << num_blocks
                           << ", setting done" << logendl;

                hit_data_block_limit = true;
                break;
            }

            if (data_block_chunk_size > 0 &&
                num_blocks >= static_cast<size_t>(data_block_chunk_size))
            {
                hit_data_block_chunk_limit = true;
                break;
            }

            parsed_data_block_bytes = 0;

            nlohmann::json& current_block = (*target)[data_block_name_][num_blocks];
            for (auto& r_item : data_block_items_)
            {
                parsed_bytes =
                    r_item->parseItem(data, current_index, length, parsed_data_block_bytes, total_size,
                                      current_block, debug);
                //            loginf << "UGA FP2 parsed " << parsed_bytes << " target '" <<
                //            target[data_block_name_][num_blocks]
                //                      << "'" << logendl;

                parsed_data_block_bytes += parsed_bytes;
                parsed_bytes_sum += parsed_bytes;
                current_index += parsed_bytes;
            }

            // loginf << "UGA2 target block '" << target[data_block_name_][num_blocks].dump(4) <<
            // "'" << logendl;

            //        loginf << "UGA AP3 parsed " << parsed_bytes_sum << " length " << length << "
            //        block "
            //        << num_blocks
            //               << " target '" << target[data_block_name_][num_blocks] << "'" <<
            //               logendl;

            ++num_blocks;
        }
    }
    catch (std::exception& e)
    {
        logerr << "asterix parser finding data blocks caught exception '" << e.what()
               << "' at index " << index << " length " << length << ", current index "
               << current_index << ", breaking " << num_blocks << logendl;

        error = true;
    }

    // loginf << "UGA3 target '" << target.dump(4) << "' parsed " << parsed_bytes_sum << " size " <<
    // length << logendl;

    //    {
    //        "cnt": 0,
    //        "content": {
    //            "index": 134,
    //            "length": 56,
    //            "record": {
    //                "category": 1,
    //                "content": {
    //                    "index": 137,
    //                    "length": 42
    //                },
    //                "length": 45
    //            }
    //        },
    //        "frame_length": 56,
    //        "frame_relative_time_ms": 2117
    //    }

    if (debug)
        loginf << "ASTERIXParser: findDataBlocks done parsed bytes " << parsed_bytes_sum
               << " num blocks " << num_blocks << " error " << error << " data block limit hit "
               << hit_data_block_limit << " data block chunk limit hit "
               << hit_data_block_chunk_limit << logendl;

    if (error)
        return std::make_tuple(parsed_bytes_sum, num_blocks, true, true);  // error and done
    else
        return std::make_tuple(parsed_bytes_sum, num_blocks, false,
                               hit_data_block_limit ? true : !hit_data_block_chunk_limit);
}

std::pair<size_t, size_t> ASTERIXParser::decodeDataBlocks(const char* data, size_t total_size,
                                                          nlohmann::json& data_blocks, bool debug)
{
    if (debug)
        loginf << "ASTERIXParser: decodeDataBlocks" << logendl;

    std::pair<size_t, size_t> ret{0, 0};  // num records, num errors

    if (!data_blocks.is_array())
        throw runtime_error("asterix parser data blocks is not an array");

    size_t num_data_blocks = data_blocks.size();

    std::vector<std::pair<size_t, size_t>> num_records;
    num_records.resize(num_data_blocks, {0, 0});

    if (debug || single_thread)  // switch to single thread in debug
    {
        for (size_t cnt = 0; cnt < num_data_blocks; ++cnt)
            num_records.at(cnt) = decodeDataBlock(data, total_size, data_blocks[cnt], debug);
    }
    else
    {
        tbb::parallel_for(size_t(0), num_data_blocks, [&](size_t cnt) {
            num_records.at(cnt) = decodeDataBlock(data, total_size, data_blocks[cnt], debug);
        });
    }

    //    for (auto num_record_it : num_records)
    //    {
    //        ret.first += num_record_it.first;
    //        ret.second += num_record_it.second;
    //    }

    auto limitJsonDump = [](const nlohmann::json& json_obj, size_t max_len = 500) -> std::string
    {
        std::string result = json_obj.dump(-1);  // -1 = compact format
        if (result.length() > max_len)
        {
            result = result.substr(0, max_len - 3) + "...";
        }
        return result;
    };

    size_t cnt = 0;
    for (std::vector<std::pair<size_t, size_t>>::iterator it = num_records.begin();
         it != num_records.end(); ++it)
    {
        ret.first += it->first;
        ret.second += it->second;

        if (it->second)
        {
            logerr << "asterix parser reported error in record '" << limitJsonDump(data_blocks[cnt]) << "'"
                   << logendl;

            if (debug && it != num_records.begin())
                loginf << "previous record " << limitJsonDump(data_blocks[cnt - 1]) << "'" << logendl;
        }

        ++cnt;
    }

    if (debug)
        loginf << "ASTERIXParser: decodeDataBlocks: done" << logendl;

    return ret;
}

std::pair<size_t, size_t> ASTERIXParser::decodeDataBlock(const char* data, size_t total_size,
                                                         nlohmann::json& data_block, bool debug)
{
    if (debug)
        loginf << "ASTERIXParser: decodeDataBlock" << logendl;

    unsigned int num_records {0}, num_errors{0};

    // check record information
    // json& record = target.at(data_block_name_);

    //        {
    //            "category": 1,
    //            "content": {
    //                "index": 3265,
    //                "length": 52
    //            },
    //            "length": 55
    //        }

    if (!data_block.contains("category"))
    {
        loginf << "asterix parser data block '" << data_block.dump(4)
               << "' does not contain category information" << logendl;
        return {0, 1};
    }

    unsigned int cat = data_block.at("category");

    if (!data_block.contains("content"))
    {
        loginf << "asterix parser data block '" << data_block.dump(4)
               << "' does not contain content information" << logendl;
        return {0, 1};
    }

    json& data_block_content = data_block.at("content");

    if (!data_block_content.contains("index"))
    {
        loginf << "asterix parser data block '" << data_block.dump(4)
               << "' does not contain content index information" << logendl;
        return {0, 1};
    }

    size_t data_block_index = data_block_content.at("index");

    if (!data_block_content.contains("length"))
    {
        loginf << "asterix parser data block '" << data_block.dump(4)
               << "' does not contain content length information" << logendl;
        return {0, 1};
    }

    size_t data_block_length = data_block_content.at("length");

    if (debug)
        loginf << "ASTERIXParser: decodeDataBlock: index " << data_block_index << " length "
               << data_block_length << " data '"
               << binary2hex((const unsigned char*)&data[data_block_index], data_block_length)
               << "'" << logendl;

    // try to decode
    if (records_.count(cat) != 0)
    {
        size_t data_block_parsed_bytes{0};
        size_t record_parsed_bytes{0};

        try
        {
            // decode
            if (debug)
                loginf << "asterix parser decoding record with cat " << cat << " index "
                       << data_block_index << " length " << data_block_length << logendl;

            data_block_content.emplace("records", json::array());
            json& records = data_block_content.at("records");

            // create records until end of content
            while (data_block_parsed_bytes < data_block_length)
            {
                // loginf << "asterix parser decoding record " << cnt << " parsed bytes " <<
                // parsed_bytes_record
                // << " length " << record_length;

                if (data_block_index + data_block_parsed_bytes >= total_size)
                {
                    logerr << "unexpected data block item at index "
                           << data_block_index + data_block_parsed_bytes
                           << " total_size " << total_size << ", quitting";
                    break;
                }

                json& current_record = records[num_records];

                record_parsed_bytes =
                    records_.at(cat)->parseItem(
                            data, data_block_index + data_block_parsed_bytes,
                            data_block_length - data_block_parsed_bytes,
                            data_block_parsed_bytes, total_size, current_record, debug);

                if (debug)
                    loginf << "record with cat " << cat << " index "
                           << data_block_index + data_block_parsed_bytes << " length "
                           << record_parsed_bytes << " data '"
                           << binary2hex((const unsigned char*)&data[data_block_index +
                                                                     data_block_parsed_bytes],
                                         record_parsed_bytes)
                           << "'" << logendl;

                if (debug)
                    loginf << "asterix parser decoding record with cat " << cat << " index "
                           << data_block_index << ": " << current_record.dump(4) << "'"
                           << logendl;

#if USE_OPENSSL
                if (add_artas_md5_hash)
                    calculateARTASMD5Hash(&data[data_block_index + data_block_parsed_bytes],
                                          record_parsed_bytes, current_record);
#endif
                if (add_record_data)
                    current_record["record_data"] = binary2hex(
                        (const unsigned char*)&data[data_block_index + data_block_parsed_bytes],
                        record_parsed_bytes);

                current_record["index"] = data_block_index + data_block_parsed_bytes;
                current_record["length"] = record_parsed_bytes;

                data_block_parsed_bytes += record_parsed_bytes;

                ++num_records;
            }
        }
        catch (std::exception& e)
        {
            std::string record_json;

            auto limitJsonDump = [](const nlohmann::json& json_obj, size_t max_len = 500) -> std::string
            {
                std::string result = json_obj.dump(-1);  // -1 = compact format
                if (result.length() > max_len)
                {
                    result = result.substr(0, max_len - 3) + "...";
                }
                return result;
            };

            if (data_block_content.contains("records") && data_block_content.at("records").size())
            {
                data_block_content.at("records").back()["error"] = true; // set error flag

                record_json = limitJsonDump(data_block_content.at("records").back());
            }

            logerr << "asterix parser decoding of cat " << cat << " failed with exception: '"
                   << e.what() << "'"
                   << "' after index " << data_block_index + data_block_parsed_bytes
                   << " data block " << binary2hex((const unsigned char*)&data[data_block_index], data_block_length)
                   << (record_json.size() ? " record json '" + record_json + "'" : "")
                   << logendl;

            ++num_errors;
        }
    }
    else if (debug)
        loginf << "asterix parser decoding record with cat " << cat << " index " << data_block_index
               << " length " << data_block_length << " skipped since cat definition is missing "
               << logendl;

    if (num_records && mappings_.count(cat))
    {
        if (debug)
            loginf << "asterix parser decoding mapping cat " << cat << ", num records " << num_records
                   << logendl;

        std::shared_ptr<Mapping> current_mapping = mappings_.at(cat);
        json& mapping_src = data_block_content.at("records");
        json mapping_dest = json::array();

        for (size_t cnt = 0; cnt < num_records; ++cnt)
            current_mapping->map(mapping_src[cnt], mapping_dest[cnt]);

        mapping_src = std::move(mapping_dest);
    }

    if (debug)
        loginf << "ASTERIXParser: decodeDataBlock: done num records " << num_records << " errors "
               << num_errors << logendl;

    return std::pair<size_t, size_t>{num_records, num_errors};
}

#if USE_OPENSSL

void ASTERIXParser::calculateARTASMD5Hash(const char* data, size_t length, nlohmann::json& target)
{
    unsigned char digest[MD5_DIGEST_LENGTH];

    // unsigned char *md5_hash =
    MD5((const unsigned char*)data, length, digest);

    // The target report identifier is the result of the concatenation of the 1st, 5th, 9th, and
    // 13th byte of the MD5 message digest algorithm of the input target report.

    // target["org"] = binary2hex((const unsigned char*)data, length);
    // target["md5"] = binary2hex(digest, MD5_DIGEST_LENGTH);
    // loginf << "ASTERIXParser: calculateARTASMD5Hash: artas " << binary2hex(digest,
    // MD5_DIGEST_LENGTH) << logendl;

    // digest[0] = digest[0];
    digest[1] = digest[4];
    digest[2] = digest[8];
    digest[3] = digest[12];

    // loginf << "ASTERIXParser: calculateARTASMD5Hash: md5 " << binary2hex(digest, 4) << logendl;

    target.emplace("artas_md5", binary2hex(digest, 4));
}

#endif

}  // namespace jASTERIX
