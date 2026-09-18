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

#include <jasterix/iteminfo.h>

#include "json.hpp"

#include <cstddef>
#include <sstream>
#include <cassert>
#include <functional>
#include <string>
#include <vector>

namespace jASTERIX
{

class ItemParserBase;

// Callback invoked for each leaf parser during setupColumnWriters.
// The caller uses it to create column arrays and inject pointers.
// Returns a pointer to the created column array. When leaf is nullptr, only the
// column is created and returned (no pointer injection) - used by parsers that
// need additional columns beyond their main one (e.g. repetitive REP counts).
using LeafSetupCallback = std::function<nlohmann::json*(ItemParserBase* leaf,
                                                        const std::string& long_name)>;

class ItemParserBase
{
public:
    ItemParserBase(const nlohmann::json& item_definition, const std::string& long_name_prefix="");
    virtual ~ItemParserBase() {}

    static ItemParserBase* createItemParser(const nlohmann::json& item_definition,
                                            const std::string& long_name_prefix);

    // Parse binary ASTERIX data into JSON. Returns number of bytes consumed.
    // @param data                Pointer to the full binary data buffer
    // @param index               Absolute byte offset where this item starts in 'data'
    // @param size                Remaining bytes in the current data block content
    // @param current_parsed_bytes  Bytes parsed so far within the enclosing record/REF/SPF
    // @param total_size          Total buffer size - hard upper bound for any data[offset] access
    // @param target              JSON object to write parsed values into
    // @param debug               Enable debug logging
    virtual size_t parseItem(const char* data, size_t index, size_t size,
                             size_t current_parsed_bytes, size_t total_size,
                             nlohmann::json& target, bool debug) = 0;

    // Encode JSON back to binary ASTERIX. Returns number of bytes written.
    // @param source    JSON object containing the values to encode
    // @param target    Pre-allocated output buffer to write binary data into
    // @param max_size  Available space in the target buffer
    // @param debug     Enable debug logging
    virtual size_t encodeItem(const nlohmann::json& source, char* target,
                              size_t max_size, bool debug) = 0;
    std::string name() const;
    std::string longNamePrefix() const;
    std::string longName() const;
    std::string type() const;

    virtual void addInfo (const std::string& edition, CategoryItemInfo& info) const;

    // Columnar output mode: walk parser tree, call callback for each leaf.
    // Default impl is a no-op (for parsers like SkipBytes that produce no output).
    virtual void setupColumnWriters(const LeafSetupCallback& callback);

    // Leaves columnar mode: drops the column targets and the captured columns, so a later
    // structured decode on the same parser tree writes into the records again. Containers
    // override this to recurse into their items and to reset their column mode.
    virtual void clearColumnWriters();

    // Inject column target for this parser (called by LeafSetupCallback).
    void setColumnTarget(nlohmann::json* column_array, size_t* record_index);

    // Columnar mode: also write this leaf's value into the record scratch object. Only for
    // the few leaves the parser logic reads back (SAC/SIC, Time of Day, conditional UAP key).
    void setScratchCapture(bool capture) { scratch_capture_ = capture; }

    // Reset the captured columns' cells of the current record to null. Discards the
    // flat-mode output of a partial REF/SPF decode whose content did not match the
    // definition. Only effective on parsers that set up their column writers via
    // captureColumns(); no-op otherwise.
    void clearCapturedColumnCells();

    // Leaves inside a repetitive item append to a per-record array cell instead of
    // assigning a scalar (struct-of-arrays, aligned by repetition index).
    void setColumnArrayAppend(bool append);

    // Shared record counter injected via setColumnTarget (nullptr when not in columnar mode)
    size_t* recordIndex() const { return record_index_; }

protected:
    // Wrap a LeafSetupCallback so all columns created for this parser's subtree are
    // remembered for later cell cleanup via clearCapturedColumnCells(). The returned
    // callback must not outlive the wrapped one (use within setupColumnWriters only).
    LeafSetupCallback captureColumns(const LeafSetupCallback& callback);

    // Write a parsed value: to column array in columnar mode, or to target in structured mode.
    // In columnar mode, also writes to target (scratch json) for conditional UAP lookups.
    template<typename T>
    void writeValue(nlohmann::json& target, T&& value)
    {
        if (column_target_)
        {
            if (scratch_capture_)
                target.emplace(name_, value);  // read back by the parser logic
            if (column_array_append_)
                (*column_target_)[*record_index_].push_back(std::forward<T>(value));
            else
                (*column_target_)[*record_index_] = std::forward<T>(value);
        }
        else
            target.emplace(name_, std::forward<T>(value));
    }

    const nlohmann::json& item_definition_;
    std::string name_;
    std::string long_name_prefix_;
    std::string long_name_;
    std::string type_;

    // Columnar output mode members
    nlohmann::json* column_target_ = nullptr;  // pointer to this leaf's column array
    size_t* record_index_ = nullptr;           // shared pointer to current record counter
    bool column_mode_ = false;                 // true when columnar mode is active (set on containers)
    bool column_array_append_ = false;         // leaf inside repetitive: append to array cell
    bool scratch_capture_ = false;             // also copy the value into the record scratch

    // Columns of this parser's subtree captured via captureColumns(), plus the shared
    // record counter, for clearCapturedColumnCells()
    std::vector<nlohmann::json*> captured_columns_;
    size_t* captured_record_index_ = nullptr;
};

bool variableHasValue(const nlohmann::json& data,
                      const std::vector<std::string>& variable_name_parts,
                      const nlohmann::json& variable_value);

inline unsigned char reverseBits(unsigned char b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;

    return b;
}

inline std::vector<std::string> split(const std::string& s, char delim)
{
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

}  // namespace jASTERIX


