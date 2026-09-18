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


#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

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

void ASTERIXParser::setFlatRecordIndices(std::map<unsigned int, size_t>* indices)
{
    flat_record_indices_ = indices;
}

void ASTERIXParser::setFlatHashColumns(std::map<unsigned int, json*>* columns)
{
    flat_hash_columns_ = columns;
}

void ASTERIXParser::setFlatRecordDataColumns(std::map<unsigned int, json*>* columns)
{
    flat_record_data_columns_ = columns;
}

void ASTERIXParser::setFlatData(std::map<unsigned int, json>* data)
{
    flat_data_ = data;
}

void ASTERIXParser::setFlatDataBlockKeyColumns(
    std::map<unsigned int, std::map<std::string, nlohmann::json*>>* columns)
{
    flat_data_block_key_columns_ = columns;
}

/**
 * Removes the cells of a rejected record from all columns of the category.
 *
 * The item parsers write leaf values at the current record index while parsing, so a record
 * that is refused afterwards has already left values behind. The record index is only
 * advanced for accepted records, so truncating every column to it restores the invariant
 * of one entry per accepted record.
 */
void ASTERIXParser::dropFlatRecord(unsigned int cat)
{
    if (!flat_data_ || !flat_record_indices_ || !flat_data_->count(cat)
        || !flat_record_indices_->count(cat))
        return;

    size_t num_accepted = flat_record_indices_->at(cat);

    for (auto& column : flat_data_->at(cat))
    {
        if (!column.is_array() || column.size() <= num_accepted)
            continue;

        column.erase(column.begin() + num_accepted, column.end());
    }
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

    for (size_t cnt = 0; cnt < num_data_blocks; ++cnt)
        num_records.at(cnt) = decodeDataBlock(data, total_size, data_blocks[cnt], debug);

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
               << binary2hex_bounded((const unsigned char*)data, data_block_index, data_block_length, total_size)
               << "'" << logendl;

    constexpr double tod_24h = 86400.0;
    constexpr double tod_trunc_period = 512.0;  // period of I001/141 truncated Time of Day

    // try to decode
    if (records_.count(cat) != 0)
    {
        size_t data_block_parsed_bytes{0};
        size_t record_parsed_bytes{0};

        if (flat_record_indices_ && flat_record_indices_->count(cat))
        {
            // flat/columnar mode - no records array, leaves write to column arrays
            try
            {
                if (debug)
                    loginf << "asterix parser decoding flat record with cat " << cat << " index "
                           << data_block_index << " length " << data_block_length << logendl;

                json record_scratch;

                // CAT001 allows SAC/SIC (item 010) only in the first record of a data block
                // (see EUROCONTROL-SPEC-0149-2a, §5.3.2.1). Track last-seen values so we
                // can inject them into subsequent records that omit item 010.
                json cat001_sac_sic;

                while (data_block_parsed_bytes < data_block_length)
                {
                    if (data_block_index + data_block_parsed_bytes >= total_size)
                    {
                        logerr << "unexpected data block item at index "
                               << data_block_index + data_block_parsed_bytes
                               << " total_size " << total_size << ", quitting";
                        break;
                    }

                    record_scratch.clear();

                    record_parsed_bytes =
                        records_.at(cat)->parseItem(
                                data, data_block_index + data_block_parsed_bytes,
                                data_block_length - data_block_parsed_bytes,
                                data_block_parsed_bytes, total_size, record_scratch, debug);

                    if (record_scratch.contains("ref_error"))
                        ++num_ref_errors_;
                    if (record_scratch.contains("spf_error"))
                        ++num_spf_errors_;

                    // CAT001: propagate SAC/SIC from first record to subsequent records
                    // that omit item 010 within the same data block.
                    // In flat/columnar mode the ItemParser skips creating the "010"
                    // sub-object - leaf values (SAC, SIC) are written directly into
                    // record_scratch, so we check for "SAC" instead of "010".
                    if (cat == 1)
                    {
                        if (record_scratch.contains("SAC"))
                        {
                            cat001_sac_sic["SAC"] = record_scratch.at("SAC");
                            cat001_sac_sic["SIC"] = record_scratch.at("SIC");
                        }
                        else if (!cat001_sac_sic.is_null() && flat_data_ && flat_data_->count(cat))
                        {
                            auto& cat_cols = flat_data_->at(cat);
                            size_t ri = flat_record_indices_->at(cat);

                            if (cat001_sac_sic.contains("SAC") && cat_cols.contains("010.SAC"))
                                cat_cols.at("010.SAC")[ri] = cat001_sac_sic.at("SAC");
                            if (cat001_sac_sic.contains("SIC") && cat_cols.contains("010.SIC"))
                                cat_cols.at("010.SIC")[ri] = cat001_sac_sic.at("SIC");
                        }

                        // Reconstruct CAT001 truncated time using last CAT002 full
                        // Time of Day for the same data source. In flat mode the
                        // data block / record ordering is lost, so the consumer
                        // cannot perform this correction itself.
                        if (flat_data_ && flat_data_->count(1))
                        {
                            // Determine SAC/SIC - either from this record or propagated
                            size_t sac = 0, sic = 0;
                            bool have_source = false;

                            if (record_scratch.contains("SAC"))
                            {
                                sac = record_scratch.at("SAC").get<size_t>();
                                sic = record_scratch.at("SIC").get<size_t>();
                                have_source = true;
                            }
                            else if (!cat001_sac_sic.is_null()
                                     && cat001_sac_sic.contains("SAC"))
                            {
                                sac = cat001_sac_sic.at("SAC").get<size_t>();
                                sic = cat001_sac_sic.at("SIC").get<size_t>();
                                have_source = true;
                            }

                            if (have_source)
                            {
                                std::string source_id =
                                    to_string(sac) + "/" + to_string(sic);

                                auto ref_it = cat002_last_tod_.find(source_id);

                                if (ref_it != cat002_last_tod_.end())
                                {
                                    auto& cat_cols = flat_data_->at(1);
                                    size_t ri = flat_record_indices_->at(1);
                                    const std::string src_col =
                                        "141.Truncated Time of Day";
                                    const std::string dst_col =
                                        "140.Time-of-Day";

                                    bool has_trunc = cat_cols.contains(src_col)
                                        && !cat_cols.at(src_col)[ri].is_null();

                                    if (has_trunc)
                                    {
                                        double t_trunc = cat_cols.at(src_col)[ri].get<double>();

                                        // I001/141 is the full Time of Day truncated to
                                        // 16 bit at LSB 1/128 s, i.e. the full time modulo
                                        // 512 s (CAT001 Part 2a section 5.2.15). Values
                                        // outside [0, 512) cannot occur in valid data;
                                        // reconstruction is impossible for them.
                                        if (t_trunc < 0 || t_trunc >= tod_trunc_period)
                                            cat_cols[dst_col][ri] = nullptr;
                                        else
                                        {
                                            // Reconstruct the full Time of Day from the
                                            // truncated value using the last I002/030 Time
                                            // of Day of the same SAC/SIC as reference, as
                                            // recommended in CAT001 Part 2a section 5.3.2.7.
                                            //
                                            // Per section 5.2.15 Note 1 the derivation is
                                            // guaranteed for source/sink clock offsets below
                                            // 512 s and requires "special care" at the 512 s
                                            // wrap ("all ones" to "all zeros" transition):
                                            // record and reference may lie in different
                                            // 512 s periods, so the reference's own period
                                            // and both neighboring periods are candidates.
                                            //
                                            // Per Note 2 the Time of Day resets to 0 at
                                            // midnight. Since 86400 s is not a multiple of
                                            // 512 s (86400 mod 512 = 384), the truncated
                                            // sequence is discontinuous at midnight; two
                                            // additional candidates cover records just
                                            // after the reset (t_trunc itself) and records
                                            // in the truncated last period of the day
                                            // (last period base + t_trunc).
                                            //
                                            // Of all candidates, the one closest to the
                                            // reference on the circular 24 h clock is
                                            // chosen. A unique interpretation only exists
                                            // for offsets below half a period (256 s);
                                            // beyond that the sync precondition of Note 1
                                            // is violated and null is stored.
                                            double ref = ref_it->second;

                                            double period_base =
                                                tod_trunc_period * std::floor(ref / tod_trunc_period);
                                            double last_period_base =
                                                tod_trunc_period * std::floor(tod_24h / tod_trunc_period);

                                            double best_tod = -1.0;
                                            double best_dist = std::numeric_limits<double>::max();

                                            auto add_candidate = [&](double cand)
                                            {
                                                if (cand < 0 || cand >= tod_24h)
                                                    return;

                                                double dist = std::fabs(cand - ref);
                                                dist = std::min(dist, tod_24h - dist);  // circular day distance

                                                if (dist < best_dist)
                                                {
                                                    best_dist = dist;
                                                    best_tod = cand;
                                                }
                                            };

                                            // reference period and both neighbors (5.2.15 Note 1)
                                            add_candidate(period_base - tod_trunc_period + t_trunc);
                                            add_candidate(period_base + t_trunc);
                                            add_candidate(period_base + tod_trunc_period + t_trunc);
                                            // midnight edges (5.2.15 Note 2)
                                            add_candidate(t_trunc);
                                            add_candidate(last_period_base + t_trunc);

                                            if (best_dist < tod_trunc_period / 2.0)
                                                cat_cols[dst_col][ri] = best_tod;
                                            else
                                                cat_cols[dst_col][ri] = nullptr;
                                        }
                                    }
                                    else
                                    {
                                        // no truncated time in the record: use the
                                        // reference time directly (validated on storage)
                                        cat_cols[dst_col][ri] = ref_it->second;
                                    }
                                }
                            }
                        }
                    }

                    // CAT002: store the last full I002/030 Time of Day per data source
                    // as reference for CAT001 truncated time reconstruction (CAT001
                    // Part 2a section 5.3.2.7). I002/030 resets to 0 at midnight
                    // (CAT002 Part 2b section 5.2.4 Note 1), but its 24 bit at LSB
                    // 1/128 s can encode values beyond 24 h; such values indicate a
                    // faulty sensor clock and are ignored instead of stored, so the
                    // reference always holds a valid time in [0, 86400).
                    if (cat == 2 && record_scratch.contains("SAC")
                        && record_scratch.contains("Time of Day"))
                    {
                        double tod = record_scratch.at("Time of Day").get<double>();

                        if (tod >= 0 && tod < tod_24h)
                        {
                            std::string source_id = to_string(record_scratch.at("SAC").get<size_t>()) +
                                                    "/" +
                                                    to_string(record_scratch.at("SIC").get<size_t>());

                            cat002_last_tod_[source_id] = tod;
                        }
                    }

                    // start of the record just parsed, needed by the side columns below
                    const size_t record_start = data_block_index + data_block_parsed_bytes;

                    data_block_parsed_bytes += record_parsed_bytes;

                    if (data_block_parsed_bytes > data_block_length)
                    {
                        // record overran its data block => decode error (desync)
                        logerr << "asterix parser decoding of cat " << cat
                               << " failed: record overran data block (parsed "
                               << data_block_parsed_bytes << " > " << data_block_length << ")"
                               << " after index " << data_block_index + data_block_parsed_bytes
                               << " data block "
                               << binary2hex_bounded((const unsigned char*)data, data_block_index, data_block_length, total_size)
                               << logendl;

                        ++num_errors;

                        // the record is rejected, so drop the cells the item parsers
                        // already wrote for it
                        dropFlatRecord(cat);

                        break;
                    }

                    // The side columns grow by push_back while the leaf columns are written
                    // at the record index. They are therefore filled only for accepted
                    // records, after the overrun check, so that all columns stay aligned.
#if USE_OPENSSL
                    if (add_artas_md5_hash)
                    {
                        calculateARTASMD5Hash(&data[record_start], record_parsed_bytes,
                                              record_scratch);

                        if (flat_hash_columns_ && flat_hash_columns_->count(cat))
                            flat_hash_columns_->at(cat)->push_back(record_scratch.at("artas_md5"));
                    }
                    else if (flat_hash_columns_ && flat_hash_columns_->count(cat))
                        flat_hash_columns_->at(cat)->push_back(nullptr);
#endif

                    // original record bytes as hex, same column mechanism as the hash
                    if (flat_record_data_columns_ && flat_record_data_columns_->count(cat))
                    {
                        if (add_record_data)
                            flat_record_data_columns_->at(cat)->push_back(
                                binary2hex((const unsigned char*)&data[record_start],
                                           record_parsed_bytes));
                        else
                            flat_record_data_columns_->at(cat)->push_back(nullptr);
                    }

                    // data block keys (recording_time etc.) as per-record side columns,
                    // null when the data block does not carry the key
                    if (flat_data_block_key_columns_ && flat_data_block_key_columns_->count(cat))
                    {
                        for (auto& [key, column] : flat_data_block_key_columns_->at(cat))
                        {
                            if (data_block.contains(key))
                                column->push_back(data_block.at(key));
                            else
                                column->push_back(nullptr);
                        }
                    }

                    ++flat_record_indices_->at(cat);
                    ++num_records;
                }
            }
            catch (std::exception& e)
            {
                logerr << "asterix parser decoding flat of cat " << cat << " failed with exception: '"
                       << e.what()
                       << "' after index " << data_block_index + data_block_parsed_bytes
                       << " data block " << binary2hex_bounded((const unsigned char*)data, data_block_index, data_block_length, total_size)
                       << logendl;

                ++num_errors;

                // the record that threw is rejected, drop its partially written cells
                dropFlatRecord(cat);
            }
        }
        else
        {
            // structured mode - current behavior
            try
            {
                if (debug)
                    loginf << "asterix parser decoding record with cat " << cat << " index "
                           << data_block_index << " length " << data_block_length << logendl;

                data_block_content.emplace("records", json::array());
                json& records = data_block_content.at("records");

                while (data_block_parsed_bytes < data_block_length)
                {
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

                    if (current_record.contains("ref_error"))
                        ++num_ref_errors_;
                    if (current_record.contains("spf_error"))
                        ++num_spf_errors_;

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

                    if (data_block_parsed_bytes > data_block_length)
                    {
                        // record overran its data block => decode error (desync)
                        current_record["error"] = true;

                        logerr << "asterix parser decoding of cat " << cat
                               << " failed: record overran data block (parsed "
                               << data_block_parsed_bytes << " > " << data_block_length << ")"
                               << " after index " << data_block_index + data_block_parsed_bytes
                               << " data block "
                               << binary2hex_bounded((const unsigned char*)data, data_block_index, data_block_length, total_size)
                               << logendl;

                        ++num_errors;
                        break;
                    }

                    ++num_records;
                }
            }
            catch (std::exception& e)
            {
                std::string record_json;

                auto limitJsonDump = [](const nlohmann::json& json_obj, size_t max_len = 500) -> std::string
                {
                    std::string result = json_obj.dump(-1);
                    if (result.length() > max_len)
                    {
                        result = result.substr(0, max_len - 3) + "...";
                    }
                    return result;
                };

                if (data_block_content.contains("records") && data_block_content.at("records").size())
                {
                    data_block_content.at("records").back()["error"] = true;

                    record_json = limitJsonDump(data_block_content.at("records").back(), 200);
                }

                logerr << "asterix parser decoding of cat " << cat << " failed with exception: '"
                       << e.what()
                       << "' after index " << data_block_index + data_block_parsed_bytes
                       << " data block " << binary2hex_bounded((const unsigned char*)data, data_block_index, data_block_length, total_size)
                       << (record_json.size() ? " record json '" + record_json + "'" : "")
                       << logendl;

                ++num_errors;
            }
        }

        if (data_block_parsed_bytes != data_block_length)
        {
            if (data_block_parsed_bytes > data_block_length)
                logerr << "ASTERIXParser: data block cat " << cat << ": parsed "
                       << data_block_parsed_bytes << " bytes but block content length is "
                       << data_block_length << logendl;
            else
                logerr << "ASTERIXParser: data block cat " << cat << ": parsed "
                       << data_block_parsed_bytes << " bytes but block content length is "
                       << data_block_length << " ("
                       << (data_block_length - data_block_parsed_bytes)
                       << " bytes unparsed)" << logendl;
        }
    }
    else if (debug)
        loginf << "asterix parser decoding record with cat " << cat << " index " << data_block_index
               << " length " << data_block_length << " skipped since cat definition is missing "
               << logendl;

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
