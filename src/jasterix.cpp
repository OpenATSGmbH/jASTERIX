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

#include "jasterix.h"

#include "asterixparser.h"
#include "category.h"
#include "datablockfindertask.h"
//#include "edition.h"
#include "files.h"
#include "frameparser.h"
#include "frameparsertask.h"
#include "logger.h"
#include "pcap/pcapreader.h"
#include "traced_assert.h"

#include <malloc.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <functional>
#include <limits>
#include <set>
#include <exception>
#include <fstream>
#include <iostream>
#include <thread>
#include <iomanip>


using namespace nlohmann;

namespace jASTERIX
{
int print_dump_indent = 4;
int frame_limit = -1;
int frame_chunk_size = 1000;
int data_block_limit = -1;
int data_block_chunk_size = 1000;
int data_write_size = 1;
int record_limit = -1;
bool single_thread = false;

#if USE_OPENSSL
bool add_artas_md5_hash = false;
#endif

bool add_record_data = false;

using namespace Files;
using namespace std;

const std::string FRAMING_SUBDIR = "/framings";
const std::string DATABLOCK_FILENAME = "/data_block_definition.json";
const std::string CATEGORY_SUBDIR = "/categories";
const std::string CATEGORIES_FILENAME = "/categories.json";

jASTERIX::jASTERIX(const std::string& definition_path, bool print, bool debug,
                   bool debug_exclude_framing)
    : definition_path_(definition_path),
      print_(print),
      debug_(debug),
      debug_exclude_framing_(debug_exclude_framing)
{
    // check framing definitions
    if (!directoryExists(definition_path_))
        throw invalid_argument("jASTERIX called with non-existing definition path '" +
                               definition_path_ + "'");

    framing_path_ = definition_path_ + FRAMING_SUBDIR;

    if (!directoryExists(framing_path_))
        throw invalid_argument("jASTERIX called with missing framing path '" + framing_path_ + "'");

    framings_.push_back("");  // add no framing
    std::string file_ending = ".json";
    for (std::string framing_file : Files::getFilesInDirectory(framing_path_))
    {
        size_t pos = framing_file.find(file_ending);

        if (pos != std::string::npos)  // if ends with json
        {
            // If found then erase it from string
            framing_file.erase(pos, file_ending.length());
            framings_.push_back(framing_file);
        }
    }

    data_block_definition_path_ = definition_path_ + DATABLOCK_FILENAME;

    if (!fileExists(data_block_definition_path_))
        throw invalid_argument("jASTERIX called without asterix data block definition");

            // check asterix category definitions

    if (!directoryExists(definition_path_ + CATEGORY_SUBDIR))
        throw invalid_argument("jASTERIX called with missing categories definition folder '" +
                               definition_path_ + CATEGORY_SUBDIR + "'");

    categories_definition_path_ = definition_path_ + CATEGORY_SUBDIR + CATEGORIES_FILENAME;
    if (!fileExists(categories_definition_path_))
        throw invalid_argument(
            "jASTERIX called without missing asterix categories definition path '" +
            categories_definition_path_ + "'");

    try  // asterix record definition
    {
        data_block_definition_ = json::parse(ifstream(definition_path_ + DATABLOCK_FILENAME));
    }
    catch (json::exception& e)
    {
        throw runtime_error(string{"jASTERIX parsing error in asterix data block definition: "} +
                            e.what());
    }

    try  // asterix categories list definition
    {
        categories_definition_ = json::parse(ifstream(categories_definition_path_));
    }
    catch (json::exception& e)
    {
        throw runtime_error(string{"jASTERIX parsing error in asterix categories definition: "} +
                            e.what());
    }

    if (!categories_definition_.is_object())
        throw invalid_argument(
            "jASTERIX called with non-object asterix categories list definition");

    try  // asterix category definitions
    {
        std::string cat_str;
        unsigned int cat;

        for (auto cat_def_it = categories_definition_.begin();
             cat_def_it != categories_definition_.end(); ++cat_def_it)
        {
            cat = 256;  // impossible number
            cat_str = cat_def_it.key();
            cat = static_cast<unsigned int>(stoul(cat_str));

            if (cat > 255 || category_definitions_.count(cat) != 0)
                throw invalid_argument("jASTERIX called with wrong asterix category '" + cat_str +
                                       "' in list definition");

            if (debug)
                loginf << "jASTERIX found asterix category " << cat_str << logendl;

            try
            {
                category_definitions_[cat] = std::shared_ptr<Category>(
                    new Category(cat_str, cat_def_it.value(), definition_path));

                traced_assert(category_definitions_.count(cat) == 1);
            }
            catch (json::exception& e)
            {
                throw runtime_error("jASTERIX parsing error in asterix category " + cat_str + ": " +
                                    e.what());
            }
        }
    }
    catch (json::exception& e)
    {
        throw runtime_error(string{"jASTERIX parsing error in asterix category definitions: "} +
                            e.what());
    }
}

jASTERIX::~jASTERIX()
{
    if (file_.is_open())
        file_.close();
}

bool jASTERIX::hasCategory(unsigned int cat) { return category_definitions_.count(cat) == 1; }

bool jASTERIX::decodeCategory(unsigned int cat)
{
    traced_assert(hasCategory(cat));
    return category_definitions_.at(cat)->decode();
}

void jASTERIX::setDecodeCategory(unsigned int cat, bool decode)
{
    traced_assert(hasCategory(cat));
    category_definitions_.at(cat)->decode(decode);
}

void jASTERIX::decodeNoCategories()
{
    for (auto& cat_it : category_definitions_)
        cat_it.second->decode(false);
}

std::shared_ptr<Category> jASTERIX::category(unsigned int cat)
{
    traced_assert(hasCategory(cat));
    return category_definitions_.at(cat);
}


std::unique_ptr<nlohmann::json> jASTERIX::analyzeFile(
    const std::string& filename, const std::string& framing_str, unsigned int record_limit)
{
    loginf << "jASTERIX: analyzeFile: filename '" << filename << "' framing '" << framing_str
           << "' record_limit " << record_limit << logendl;

    return runAnalysis([&]() { decodeFile(filename, framing_str, nullptr, true); }, record_limit);
}

std::unique_ptr<nlohmann::json> jASTERIX::analyzeFile(const std::string& filename, unsigned int record_limit)
{
    loginf << "jASTERIX: analyzeFile: filename '" << filename << "' record_limit " << record_limit << logendl;

    size_t file_size = openFile(filename);

    const char* data = file_.data();

    std::unique_ptr<nlohmann::json> analysis_result = analyzeData(data, file_size, record_limit);

    file_.close();

    return analysis_result;
}

std::string jASTERIX::analyzeFileCSV(const std::string& filename, const std::string& framing_str,
                                     unsigned int record_limit)
{
    std::unique_ptr<nlohmann::json> analysis_result = analyzeFile(filename, framing_str, record_limit);

            // sac/sic -> cat -> count
//    traced_assert(analysis_result->contains("sensor_counts"));

    std::stringstream ss;

//    std::map<std::string, std::map<std::string, unsigned int>> sensor_counts = analysis_result->at("sensor_counts");

//    ss << "sensor counts" << endl;

//    ss << "sensor;cat;count" << endl;

//    for (const auto& sen_it : sensor_counts)
//        for (const auto& count_it : sen_it.second)
//            ss << sen_it.first <<  ";" << count_it.first << ";" << count_it.second << endl;

//    ss << endl << endl;

//            // cat -> key -> count/min/max
//    traced_assert(analysis_result->contains("data_items"));

//    ss << "data items" << endl;

    ss << toCSV(*analysis_result);

    return ss.str();
}

std::string jASTERIX::analyzeFileCSV(const std::string& filename, unsigned int record_limit)
{
    std::unique_ptr<nlohmann::json> analysis_result = analyzeFile(filename, record_limit);

    std::stringstream ss;

    unsigned int num_errors=0;
    if (analysis_result->contains("num_errors"))
    {
        num_errors = analysis_result->at("num_errors");
        analysis_result->erase("num_errors");
    }

    ss << "num_errors;" << num_errors << endl;

    unsigned int num_records=0;
    if (analysis_result->contains("num_records"))
    {
        num_records = analysis_result->at("num_records");
        analysis_result->erase("num_records");
    }

    ss << "num_records;" << num_records << endl;

    //loginf << "jASTERIX: analyzeFileCSV: analysis_result '" << analysis_result->dump(2) << "'";

    ss << toCSV(*analysis_result);

    //loginf << "jASTERIX: analyzeFileCSV: toCSV '" << ss.str() << "'";

    return ss.str();
}

std::unique_ptr<nlohmann::json> jASTERIX::analyzePCAPFile(const std::string& filename,
                                                          unsigned int record_limit)
{
    loginf << "jASTERIX: analyzePCAPFile: filename '" << filename << "' record_limit "
           << record_limit << logendl;

    PcapReader reader;

    if (!reader.open(filename))
        throw std::runtime_error("jASTERIX unable to open PCAP file '" + filename + "'");

    // accumulate payload per network stream (signature), so each can be probed individually
    std::map<PcapReader::Signature, PcapReader::StreamData> streams = reader.readPerSignature();

    std::unique_ptr<nlohmann::json> result {new nlohmann::json()};

    for (auto& stream_it : streams)
    {
        const std::string sig_str = PcapReader::signatureToString(stream_it.first);
        PcapReader::StreamData& stream = stream_it.second;

        if (stream.data.empty())
            continue;

        // reset per-invocation counters so each stream is analyzed independently
        num_records_ = 0;
        num_errors_  = 0;
        num_ref_errors_ = 0;
        num_spf_errors_ = 0;

        std::unique_ptr<nlohmann::json> stream_result =
            analyzeData(stream.data.data(), stream.data.size(), record_limit);

        (*result)[sig_str] = std::move(*stream_result);

        // first/last network (capture) timestamp of the stream, as date/time
        if (stream.has_time)
        {
            (*result)[sig_str]["first_time"]       = PcapReader::timeToString(stream.first_time);
            (*result)[sig_str]["last_time"]        = PcapReader::timeToString(stream.last_time);
            (*result)[sig_str]["first_time_epoch"] = stream.first_time;
            (*result)[sig_str]["last_time_epoch"]  = stream.last_time;
        }
    }

    if (reader.hasUnknownHeaders())
        (*result)["unknown_packet_headers"] = true;

    return result;
}

std::string jASTERIX::analyzePCAPFileCSV(const std::string& filename, unsigned int record_limit)
{
    std::unique_ptr<nlohmann::json> analysis_result = analyzePCAPFile(filename, record_limit);

    std::stringstream ss;

    for (auto& sig_it : analysis_result->items())
    {
        if (!sig_it.value().is_object())  // skip scalar entries like "unknown_packet_headers"
            continue;

        ss << "signature;" << sig_it.key() << endl;

        nlohmann::json stream_result = sig_it.value();

        unsigned int num_errors = 0;
        if (stream_result.contains("num_errors"))
        {
            num_errors = stream_result.at("num_errors");
            stream_result.erase("num_errors");
        }
        ss << "num_errors;" << num_errors << endl;

        unsigned int num_records = 0;
        if (stream_result.contains("num_records"))
        {
            num_records = stream_result.at("num_records");
            stream_result.erase("num_records");
        }
        ss << "num_records;" << num_records << endl;

        if (stream_result.contains("first_time"))
        {
            ss << "first_time;" << stream_result.at("first_time").get<std::string>() << endl;
            stream_result.erase("first_time");
        }
        if (stream_result.contains("last_time"))
        {
            ss << "last_time;" << stream_result.at("last_time").get<std::string>() << endl;
            stream_result.erase("last_time");
        }
        stream_result.erase("first_time_epoch");
        stream_result.erase("last_time_epoch");

        ss << toCSV(stream_result);
        ss << endl;
    }

    return ss.str();
}



std::unique_ptr<nlohmann::json> jASTERIX::analyzeData(const char* data, unsigned int total_size,
                                                      unsigned int record_limit)
{
    return runAnalysis([&]() { decodeData(data, total_size, nullptr, true, true); }, record_limit);
}

std::string jASTERIX::analyzeDataCSV(const char* data, unsigned int total_size,
                                     unsigned int record_limit)
{
    std::unique_ptr<nlohmann::json> analysis_result = analyzeData(data, total_size, record_limit);

            // sac/sic -> cat -> count
//    traced_assert(analysis_result->contains("sensor_counts"));

    std::stringstream ss;

//    std::map<std::string, std::map<std::string, unsigned int>> sensor_counts = analysis_result->at("sensor_counts");

//    ss << "sensor counts" << endl;

//    ss << "sensor;cat;count" << endl;

//    for (const auto& sen_it : sensor_counts)
//        for (const auto& count_it : sen_it.second)
//            ss << sen_it.first <<  ";" << count_it.first << ";" << count_it.second << endl;

//    ss << endl << endl;

//            // cat -> key -> count/min/max
//    traced_assert(analysis_result->contains("data_items"));

//    ss << "data items" << endl;

    ss << toCSV(*analysis_result);

    return ss.str();
}

std::string jASTERIX::toCSV(const nlohmann::json& analysis_result)
{
    // sac/sic -> cat -> key -> count/min/max, the counters and skipped categories are not listed

    std::stringstream ss;

    ss << "sac/sic;name;count;min;max" << endl;

    string cat_str;

    for (const auto& sensor_it : analysis_result.items())
    {
        if (!sensor_it.value().is_object() || sensor_it.key() == "skipped_categories")
            continue;

        for (const auto& cat_it : sensor_it.value().items())
        {
            if (!cat_it.value().is_object())
                continue;

            std::ostringstream oss;
            oss << std::setw(3) << std::setfill('0') << cat_it.key();
            cat_str = oss.str();

            ss << sensor_it.key() << " CAT" << cat_str << ":" << endl;

            for (const auto& di_info_it : cat_it.value().items()) // key -> count/min/max
            {
                if (di_info_it.value().is_primitive())
                {
                    ss << sensor_it.key() << ";count;" << di_info_it.value() << ";;" << endl;

                    continue;
                }

                ss << sensor_it.key() << ";" << di_info_it.key() << ";" << di_info_it.value().at("count");
                ss << ";";

                if (di_info_it.value().contains("min"))
                    ss << di_info_it.value().at("min");

                ss << ";";

                if (di_info_it.value().contains("max"))
                    ss << di_info_it.value().at("max");

                ss << endl;
            }

            ss << endl << endl;
        }
    }

    return ss.str();
}

void jASTERIX::setupFlatColumns()
{
    flat_data_.clear();
    flat_hash_columns_.clear();
    flat_record_data_columns_.clear();
    flat_data_block_key_columns_.clear();

    for (auto& [cat, cat_def] : category_definitions_)
    {
        if (cat_def->decode())
        {
            flat_record_indices_[cat] = 0;
            cat_def->setupColumnWriters([this, cat](ItemParserBase* leaf, const std::string& name) -> nlohmann::json* {
                flat_data_[cat][name] = nlohmann::json::array();
                if (leaf)
                    leaf->setColumnTarget(&flat_data_[cat][name], &flat_record_indices_[cat]);
                return &flat_data_[cat][name];
            });

#if USE_OPENSSL
            if (add_artas_md5_hash)
            {
                flat_data_[cat]["artas_md5"] = nlohmann::json::array();
                flat_hash_columns_[cat] = &flat_data_[cat]["artas_md5"];
            }
#endif
            if (add_record_data)
            {
                flat_data_[cat]["record_data"] = nlohmann::json::array();
                flat_record_data_columns_[cat] = &flat_data_[cat]["record_data"];
            }

            // recording time keys of the current source, copied per record from the data block
            for (const std::string& key : flat_data_block_keys_)
            {
                flat_data_[cat][key] = nlohmann::json::array();
                flat_data_block_key_columns_[cat][key] = &flat_data_[cat][key];
            }
        }
    }
}

void jASTERIX::clearFlatColumns()
{
    for (auto& [cat, cat_def] : category_definitions_)
        cat_def->clearColumnWriters();

    flat_data_.clear();
    flat_record_indices_.clear();
    flat_hash_columns_.clear();
    flat_record_data_columns_.clear();
    flat_data_block_key_columns_.clear();
}

std::unique_ptr<nlohmann::json> jASTERIX::moveFlatData()
{
    auto result = std::make_unique<nlohmann::json>();
    for (auto& [cat, cat_data] : flat_data_)
    {
        auto idx_it = flat_record_indices_.find(cat);
        if (idx_it == flat_record_indices_.end() || idx_it->second == 0)
            continue;

        size_t num_records = idx_it->second;

        nlohmann::json filtered_cat = nlohmann::json::object();
        for (auto it = cat_data.begin(); it != cat_data.end(); ++it)
        {
            nlohmann::json& column = it.value();

            // never written in this chunk
            if (!column.is_array() || column.empty())
                continue;

            filtered_cat[it.key()] = std::move(column);

            // the parsers keep their pointers to this column, so it stays in place: the
            // moved-from value becomes an empty array again, with room for a chunk of the
            // size just delivered. No parser tree walk per chunk.
            column = nlohmann::json::array();
            column.get_ref<nlohmann::json::array_t&>().reserve(num_records);
        }
        (*result)[std::to_string(cat)] = std::move(filtered_cat);

        idx_it->second = 0;
    }

    return result;
}

void jASTERIX::decodeFile(
    const std::string& filename, const std::string& framing_str,
    decode_callback_t data_callback,
    bool do_flat)
{
    size_t file_size = openFile(filename);

    const char* data = file_.data();

    nlohmann::json framing_definition = loadFramingDefinition(framing_str);

    record_limit_base_ = num_records_;

    // recording time keys this framing provides, become flat side columns
    flat_data_block_keys_ = FrameParser::recordingKeys(framing_definition);

            resetChunkState();

    // create ASTERIX parser
    ASTERIXParser asterix_parser(data_block_definition_, category_definitions_, debug_);

    // REF/SPF fallback counts accumulate in the local parser; members stay cumulative
    // per instance like num_errors_
    size_t ref_errors_base = num_ref_errors_;
    size_t spf_errors_base = num_spf_errors_;

    if (do_flat)
    {
        flat_record_indices_.clear();
        setupFlatColumns();
        asterix_parser.setFlatRecordIndices(&flat_record_indices_);
        asterix_parser.setFlatHashColumns(&flat_hash_columns_);
        asterix_parser.setFlatRecordDataColumns(&flat_record_data_columns_);
        asterix_parser.setFlatData(&flat_data_);
        asterix_parser.setFlatDataBlockKeyColumns(&flat_data_block_key_columns_);
    }
    else
        clearFlatColumns();

            // create frame parser
    bool debug_framing = debug_ && !debug_exclude_framing_;
    FrameParser frame_parser(framing_definition, asterix_parser, debug_framing);

    nlohmann::json json_header;

    size_t index{0};

            // parsing header
    if (frame_parser.hasFileHeaderItems())
        index = frame_parser.parseHeader(data, 0, file_size, json_header, debug_framing);

    if (debug_)
        loginf << "jasterix: creating frame parser task index " << index << " header '"
               << json_header.dump(4) << "'" << logendl;

    //    FrameParserTask* task = new (tbb::task::allocate_root())
    //        FrameParserTask(*this, frame_parser, json_header, data, index, file_size, debug_framing);
    //    tbb::task::enqueue(*task);

    std::unique_ptr<FrameParserTask> task {
                                          new FrameParserTask(*this, frame_parser, json_header, data, index, file_size, debug_framing)};
    task->start();

    if (debug_)
    {
        //loginf << "jASTERIX: decodeFile: waiting on task to be finished";

        while (!task->done())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

                //loginf << "jASTERIX: decodeFile: task done";
    }

    std::unique_ptr<nlohmann::json> data_chunk;

    size_t num_callback_frames;
    size_t chunk_bytes_read{0};
    std::pair<size_t, size_t> dec_ret{0, 0};

            //loginf << "jASTERIX: decodeFile: processing";

    stop_decoding_ = false;

    while (!stop_decoding_)
    {
        traced_assert(!data_chunk);

        {
            std::unique_lock<std::mutex> lock(data_chunks_mutex_);
            data_chunks_cv_.wait(lock, [this] {
                return !data_chunks_.empty() || data_processing_done_ || stop_decoding_;
            });

            if (stop_decoding_ || data_chunks_.empty())
                break;

            chunk_bytes_read = data_chunks_.front().second;
            data_chunk = std::move(data_chunks_.front().first);
            data_chunks_.pop_front();
        }
        data_chunks_cv_.notify_one();  // wake producer from backpressure

        if (debug_)
            loginf << "jASTERIX: decoding frames" << logendl;

        num_callback_frames = data_chunk->at("frames").size();
        num_frames_ += num_callback_frames;

        try
        {
            dec_ret = frame_parser.decodeFrames(data, file_size, data_chunk.get(), debug_);
            num_records_ += dec_ret.first;
            num_errors_ += dec_ret.second;
            num_ref_errors_ = ref_errors_base + asterix_parser.numREFErrors();
            num_spf_errors_ = spf_errors_base + asterix_parser.numSPFErrors();

            if (debug_)
                loginf << "jASTERIX processing " << num_frames_ << " frames, " << num_records_
                       << " records " << num_errors_ << " errors " << logendl;

            if (do_flat)
            {
                auto flat_chunk = moveFlatData();

                if (analysis_mode_)
                    analyzeFlatChunk(*flat_chunk, *data_chunk, true);
                else
                {
                    if (print_)
                        std::cout << flat_chunk->dump(print_dump_indent) << std::endl;

                    if (data_callback)
                        data_callback(std::move(flat_chunk), chunk_bytes_read, num_callback_frames, dec_ret.first, dec_ret.second);
                }

                data_chunk = nullptr;
            }
            else
            {
                if (print_)
                    std::cout << data_chunk->dump(print_dump_indent) << std::endl;

                if (data_callback)
                    data_callback(std::move(data_chunk), chunk_bytes_read, num_callback_frames, dec_ret.first, dec_ret.second);
                else
                    data_chunk = nullptr;
            }

            if (frame_limit > 0 && num_frames_ >= static_cast<unsigned>(frame_limit))
            {
                if (debug_)
                    loginf << "jASTERIX processing hit framelimit" << logendl;

                break;
            }

            if (stopAfterChunk())
            {
                if (debug_)
                    loginf << "jASTERIX processing stops after chunk" << logendl;

                break;
            }
        }
        catch (std::exception& e)
        {
            logerr << "jASTERIX caught exception '" << e.what() << "', breaking" << logendl;

            forceStopTask(*task);

            throw;  // rethrow
        }
    }

    if (!task->done()) // aborted
        forceStopTask(*task);

    if (debug_)
        loginf << "jASTERIX decode file done" << logendl;

    file_.close();
}

void jASTERIX::decodeFile(
    const std::string& filename,
    decode_callback_t data_callback,
    bool do_flat)
{
    size_t file_size = openFile(filename);

    const char* data = file_.data();

    //@TODO: most likely we could call decodeFile(const char*, ...) here

    flat_data_block_keys_.clear();  // raw/netto file, no recording time source

    record_limit_base_ = num_records_;

    resetChunkState();

    // create ASTERIX parser
    ASTERIXParser asterix_parser(data_block_definition_, category_definitions_, debug_);

    // REF/SPF fallback counts accumulate in the local parser; members stay cumulative
    // per instance like num_errors_
    size_t ref_errors_base = num_ref_errors_;
    size_t spf_errors_base = num_spf_errors_;

    if (do_flat)
    {
        flat_record_indices_.clear();
        setupFlatColumns();
        asterix_parser.setFlatRecordIndices(&flat_record_indices_);
        asterix_parser.setFlatHashColumns(&flat_hash_columns_);
        asterix_parser.setFlatRecordDataColumns(&flat_record_data_columns_);
        asterix_parser.setFlatData(&flat_data_);
        asterix_parser.setFlatDataBlockKeyColumns(&flat_data_block_key_columns_);
    }
    else
        clearFlatColumns();

    if (debug_)
        loginf << "jASTERIX: finding data blocks" << logendl;

    size_t index{0};

    //    DataBlockFinderTask* task = new (tbb::task::allocate_root())
    //        DataBlockFinderTask(*this, asterix_parser, data, index, file_size, debug_);
    //    tbb::task::enqueue(*task);

    std::unique_ptr<DataBlockFinderTask> task {
                                              new DataBlockFinderTask(*this, asterix_parser, data, index, file_size, debug_)};

    task->start();

    if (debug_)
        while (!task->done())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

    std::unique_ptr<nlohmann::json> data_block_chunk;

    size_t chunk_bytes_read{0};
    std::pair<size_t, size_t> dec_ret{0, 0};

    stop_decoding_ = false;

    while (!stop_decoding_)
    {
        if (task->error())
        {
            ++num_errors_;
            break;
        }

        // loginf << "jasterix: task done " << data_block_processing_done_ << " empty " <<
        // data_block_chunks_.empty()
        // << logendl;

        traced_assert(!data_block_chunk);

        {
            std::unique_lock<std::mutex> lock(data_block_chunks_mutex_);
            data_block_chunks_cv_.wait(lock, [this] {
                return !data_block_chunks_.empty() || data_block_processing_done_ || stop_decoding_;
            });

            if (stop_decoding_ || data_block_chunks_.empty())
                break;

            chunk_bytes_read = data_block_chunks_.front().second;
            data_block_chunk = std::move(data_block_chunks_.front().first);
            data_block_chunks_.pop_front();
        }
        data_block_chunks_cv_.notify_one();  // wake producer from backpressure

        if (debug_)
            loginf << "jasterix: decoding data block" << logendl;

        try
        {
            if (!data_block_chunk->contains("data_blocks"))
                throw runtime_error("jasterix data blocks not found");

            if (!data_block_chunk->at("data_blocks").is_array())
                throw runtime_error("jasterix data blocks is not an array");

            dec_ret =
                asterix_parser.decodeDataBlocks(data, file_size, data_block_chunk->at("data_blocks"), debug_);
            num_records_ += dec_ret.first;
            num_errors_ += dec_ret.second;
            num_ref_errors_ = ref_errors_base + asterix_parser.numREFErrors();
            num_spf_errors_ = spf_errors_base + asterix_parser.numSPFErrors();

            if (do_flat)
            {
                auto flat_chunk = moveFlatData();

                if (analysis_mode_)
                    analyzeFlatChunk(*flat_chunk, *data_block_chunk, false);
                else
                {
                    if (print_)
                        std::cout << flat_chunk->dump(print_dump_indent) << std::endl;

                    if (data_callback)
                        data_callback(std::move(flat_chunk), chunk_bytes_read, 0, dec_ret.first, dec_ret.second);
                }

                data_block_chunk = nullptr;
            }
            else
            {
                if (print_)
                    std::cout << data_block_chunk->dump(print_dump_indent) << std::endl;

                if (data_callback)
                    data_callback(std::move(data_block_chunk), chunk_bytes_read, 0, dec_ret.first, dec_ret.second);
                else
                    data_block_chunk = nullptr;
            }

            if (stopAfterChunk())
            {
                if (debug_)
                    loginf << "jASTERIX processing stops after chunk" << logendl;

                break;
            }
        }
        catch (std::exception& e)
        {
            logerr << "jASTERIX caught exception'" << e.what() << "', breaking" << logendl;

            forceStopTask(*task);

            throw;
        }
    }

    if (!task->done()) // aborted
        forceStopTask(*task);

    if (debug_)
        loginf << "jASTERIX decode file done" << logendl;

    file_.close();
}

void jASTERIX::stopDecoding()
{
    stop_decoding_ = true;
    data_block_chunks_cv_.notify_all();
    data_chunks_cv_.notify_all();
}

void jASTERIX::notifyDataChunksError()
{
    {
        std::lock_guard<std::mutex> lock(data_chunks_mutex_);
        data_processing_done_ = true;
    }
    data_chunks_cv_.notify_all();
}

void jASTERIX::notifyDataBlockChunksError()
{
    {
        std::lock_guard<std::mutex> lock(data_block_chunks_mutex_);
        data_block_processing_done_ = true;
    }
    data_block_chunks_cv_.notify_all();
}

void jASTERIX::decodeData(const char* data,
                          unsigned int total_size,
                          decode_callback_t data_callback,
                          bool abortable,
                          bool do_flat)
{
    resetChunkState();

    ASTERIXParser asterix_parser_instance (data_block_definition_, category_definitions_, debug_);

    // REF/SPF fallback counts accumulate in the local parser; members stay cumulative
    // per instance like num_errors_
    size_t ref_errors_base = num_ref_errors_;
    size_t spf_errors_base = num_spf_errors_;

    // a PCAP source provides the capture time per data block, plain buffers provide nothing
    if (pcap_packet_times_)
        flat_data_block_keys_ = {"recording_time", "recording_date"};
    else
        flat_data_block_keys_.clear();

    // decodePCAPFile resets the record count once and calls decodeData per capture chunk,
    // its record limit spans all chunks
    if (!pcap_packet_times_)
        record_limit_base_ = num_records_;

    if (do_flat)
    {
        flat_record_indices_.clear();
        setupFlatColumns();
        asterix_parser_instance.setFlatRecordIndices(&flat_record_indices_);
        asterix_parser_instance.setFlatHashColumns(&flat_hash_columns_);
        asterix_parser_instance.setFlatRecordDataColumns(&flat_record_data_columns_);
        asterix_parser_instance.setFlatData(&flat_data_);
        asterix_parser_instance.setFlatDataBlockKeyColumns(&flat_data_block_key_columns_);
    }
    else
    {
        asterix_parser_instance.setFlatRecordIndices(nullptr);
        asterix_parser_instance.setFlatHashColumns(nullptr);
        asterix_parser_instance.setFlatRecordDataColumns(nullptr);
        asterix_parser_instance.setFlatData(nullptr);
        asterix_parser_instance.setFlatDataBlockKeyColumns(nullptr);

        clearFlatColumns();
    }

    data_block_processing_done_ = false;

    size_t index{0};

    //    DataBlockFinderTask* task = new (tbb::task::allocate_root())
    //        DataBlockFinderTask(*this, asterix_parser_instance, data, index, len, debug_);
    //    tbb::task::enqueue(*task);

    std::unique_ptr<DataBlockFinderTask> task {
                                              new DataBlockFinderTask(*this, asterix_parser_instance, data, index, total_size, debug_)};
    task->start();

    if (debug_)
        while (!task->done())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

    std::unique_ptr<nlohmann::json> data_block_chunk;

    size_t chunk_bytes_read{0};
    std::pair<size_t, size_t> dec_ret{0, 0};

    stop_decoding_ = false;

    while (!abortable || !stop_decoding_)
    {
        // loginf << "jasterix: task done " << data_block_processing_done_ << " empty " <<
        // data_block_chunks_.empty()
        // << logendl;

        traced_assert(!data_block_chunk);

        {
            std::unique_lock<std::mutex> lock(data_block_chunks_mutex_);
            data_block_chunks_cv_.wait(lock, [this, abortable] {
                return !data_block_chunks_.empty() || data_block_processing_done_
                       || (abortable && stop_decoding_);
            });

            if ((abortable && stop_decoding_) || data_block_chunks_.empty())
                break;

            chunk_bytes_read = data_block_chunks_.front().second;
            data_block_chunk = std::move(data_block_chunks_.front().first);
            data_block_chunks_.pop_front();
        }
        data_block_chunks_cv_.notify_one();  // wake producer from backpressure

        if (debug_)
            loginf << "jasterix: decoding data block" << logendl;

        try
        {
            if (!data_block_chunk->contains("data_blocks"))
                throw runtime_error("jasterix data blocks not found");

            if (!data_block_chunk->at("data_blocks").is_array())
                throw runtime_error("jasterix data blocks is not an array");

            // when decoding a PCAP, stamp each data block with its network capture time
            // before the records are decoded, so flat mode copies recording_time /
            // recording_date per record and structured output carries them per data block
            if (pcap_packet_times_)
                stampPCAPTimes(data_block_chunk->at("data_blocks"), !do_flat);

            dec_ret =
                asterix_parser_instance.decodeDataBlocks(data, total_size, data_block_chunk->at("data_blocks"), debug_);
            num_records_ += dec_ret.first;
            num_errors_ += dec_ret.second;
            num_ref_errors_ = ref_errors_base + asterix_parser_instance.numREFErrors();
            num_spf_errors_ = spf_errors_base + asterix_parser_instance.numSPFErrors();

            if (do_flat)
            {
                auto flat_chunk = moveFlatData();

                if (analysis_mode_)
                    analyzeFlatChunk(*flat_chunk, *data_block_chunk, false);
                else
                {
                    if (print_)
                        std::cout << flat_chunk->dump(print_dump_indent) << std::endl;

                    if (data_callback)
                        data_callback(std::move(flat_chunk), chunk_bytes_read, 0, dec_ret.first, dec_ret.second);
                }

                data_block_chunk = nullptr;
            }
            else
            {
                if (print_)
                    std::cout << data_block_chunk->dump(print_dump_indent) << std::endl;

                if (data_callback)
                    data_callback(std::move(data_block_chunk), chunk_bytes_read, 0, dec_ret.first, dec_ret.second);
                else
                    data_block_chunk = nullptr;
            }

            if (stopAfterChunk())
            {
                if (debug_)
                    loginf << "jASTERIX processing stops after chunk" << logendl;

                break;
            }
        }
        catch (std::exception& e)
        {
            logerr << "jASTERIX caught exception'" << e.what() << "', breaking" << logendl;

            forceStopTask(*task);

            throw;
        }
    }

    if (!task->done()) // aborted
        forceStopTask(*task);

    if (debug_)
        loginf << "jASTERIX decode data done" << logendl;
}

void jASTERIX::stampPCAPTimes(nlohmann::json& data_blocks, bool with_time_string)
{
    if (!pcap_packet_times_ || pcap_packet_times_->empty() || !data_blocks.is_array())
        return;

    const std::vector<std::pair<std::size_t, double>>& packet_times = *pcap_packet_times_;

    for (auto& data_block : data_blocks)
    {
        if (!data_block.contains("content") || !data_block.at("content").contains("index"))
            continue;

        size_t idx = data_block.at("content").at("index");

        // first packet whose payload starts after idx; the one before it contains idx
        auto it = std::upper_bound(
            packet_times.begin(), packet_times.end(), idx,
            [](size_t value, const std::pair<std::size_t, double>& p) { return value < p.first; });

        double ts = (it == packet_times.begin()) ? packet_times.front().second
                                                  : std::prev(it)->second;

        // the formatted string is only visible in structured output, flat output copies the
        // recording keys below per record
        if (with_time_string)
            data_block["pcap_time"] = PcapReader::timeToString(ts);
        data_block["pcap_time_epoch"] = ts;

        // seconds since UTC midnight and the UTC date as YYYYMMDD, same keys as the framings
        time_t secs = static_cast<time_t>(std::floor(ts));
        struct tm tm_utc;
        gmtime_r(&secs, &tm_utc);

        data_block["recording_time"] = tm_utc.tm_hour * 3600.0 + tm_utc.tm_min * 60.0
                                       + tm_utc.tm_sec + (ts - static_cast<double>(secs));
        data_block["recording_date"] = static_cast<unsigned int>(
            (tm_utc.tm_year + 1900) * 10000 + (tm_utc.tm_mon + 1) * 100 + tm_utc.tm_mday);
    }
}

void jASTERIX::decodePCAPFile(const std::string& filename,
                              decode_callback_t data_callback,
                              bool do_flat)
{
    loginf << "jASTERIX: decodePCAPFile: filename '" << filename << "'" << logendl;

    PcapReader reader;

    if (!reader.open(filename))
        throw std::runtime_error("jASTERIX unable to open PCAP file '" + filename + "'");

    // process the file in chunks of payload bytes, decoding each via decodeData (raw/netto)
    const size_t chunk_max_bytes = 4 * 1024 * 1024;

    num_frames_  = 0;
    num_records_ = 0;
    num_errors_  = 0;
    num_ref_errors_ = 0;
    num_spf_errors_ = 0;
    record_limit_base_ = 0;

    stop_decoding_ = false;

    std::vector<char> chunk;
    bool              eof = false;

    while (!eof && !stop_decoding_ && !recordLimitReached())
    {
        if (!reader.readNextChunk(chunk, chunk_max_bytes, eof))
            throw std::runtime_error("jASTERIX error reading PCAP file '" + filename + "'");

        if (chunk.empty())
            continue;

        if (debug_)
            loginf << "jASTERIX: decodePCAPFile: decoding " << chunk.size() << " payload byte(s)"
                   << logendl;

        // expose this chunk's packet offsets -> capture times so decodeData can stamp each
        // decoded data block with its network time (consumed before print/callback)
        pcap_packet_times_ = &reader.lastChunkPacketTimes();

        // each chunk is a self-contained, contiguous sequence of ASTERIX data blocks
        decodeData(chunk.data(), chunk.size(), data_callback, /*abortable*/ true, do_flat);

        pcap_packet_times_ = nullptr;
    }

    if (reader.hasUnknownHeaders())
        loginf << "jASTERIX: decodePCAPFile: encountered unknown packet headers in '" << filename
               << "'" << logendl;

    if (debug_)
        loginf << "jASTERIX: decodePCAPFile: done" << logendl;
}

size_t jASTERIX::numFrames() const { return num_frames_; }

size_t jASTERIX::numRecords() const { return num_records_; }

bool jASTERIX::recordLimitReached() const
{
    size_t limit = call_record_limit_ > 0 ? call_record_limit_
                                          : (record_limit > 0 ? static_cast<size_t>(record_limit) : 0);

    return limit > 0 && num_records_ - record_limit_base_ >= limit;
}

void jASTERIX::addDataBlockChunk(std::unique_ptr<nlohmann::json> data_block_chunk, size_t bytes_read,
                                 bool error, bool done)
{
    if (debug_)
    {
        loginf << "jASTERIX adding data block chunk, error " << error << " done " << done
               << logendl;

        if (!data_block_chunk->contains("data_blocks"))
            throw std::runtime_error(
                "jASTERIX scoped data block information contains no data blocks");

        if (!data_block_chunk->at("data_blocks").is_array())
            throw std::runtime_error("jASTERIX scoped scoped data block information is not array");
    }

    if (error)
        num_errors_ += 1;

    {
        std::lock_guard<std::mutex> lock(data_block_chunks_mutex_);
        data_block_chunks_.push_back({std::move(data_block_chunk), bytes_read});
        data_block_processing_done_ = done;
    }
    data_block_chunks_cv_.notify_one();  // wake consumer

    // backpressure: wait if queue is too full (debug forces decoding of all frames first)
    if (!done && !debug_)
    {
        std::unique_lock<std::mutex> lock(data_block_chunks_mutex_);
        data_block_chunks_cv_.wait(lock, [this] {
            return data_block_chunks_.size() < 2 || data_block_processing_done_ || debug_ || stop_decoding_;
        });
    }
}

void jASTERIX::addDataChunk(std::unique_ptr<nlohmann::json> data_chunk, size_t bytes_read, bool done)
{
    //loginf << "jASTERIX: addDataChunk: done " << done;

    if (debug_)
    {
        loginf << "jASTERIX adding data chunk, done " << done << logendl;

        if (!data_chunk->contains("frames"))
            throw std::runtime_error("jASTERIX scoped frames information contains no frames");

        if (!data_chunk->at("frames").is_array())
            throw std::runtime_error("jASTERIX scoped frames information is not array");
    }

    {
        std::lock_guard<std::mutex> lock(data_chunks_mutex_);
        data_chunks_.push_back({std::move(data_chunk), bytes_read});
        data_processing_done_ = done;
    }
    data_chunks_cv_.notify_one();  // wake consumer

    //loginf << "jASTERIX: addDataChunk: sleep";

    // backpressure: wait if queue is too full (debug forces decoding of all frames first)
    if (!done && !debug_)
    {
        std::unique_lock<std::mutex> lock(data_chunks_mutex_);
        data_chunks_cv_.wait(lock, [this] {
            return data_chunks_.size() < 2 || data_processing_done_ || debug_ || stop_decoding_;
        });
    }

    //loginf << "jASTERIX: addDataChunk: done";
}

const std::string& jASTERIX::dataBlockDefinitionPath() const { return data_block_definition_path_; }

const std::string& jASTERIX::categoriesDefinitionPath() const
{
    return categories_definition_path_;
}

const std::string& jASTERIX::framingsFolderPath() const { return framing_path_; }

void jASTERIX::setDebug(bool debug) { debug_ = debug; }

size_t jASTERIX::numErrors() const { return num_errors_; }

size_t jASTERIX::numREFErrors() const { return num_ref_errors_; }

size_t jASTERIX::numSPFErrors() const { return num_spf_errors_; }

size_t jASTERIX::openFile (const std::string& filename)
{
    // check and open file
    if (!fileExists(filename))
        throw invalid_argument("jASTERIX called with non-existing file '" + filename + "'");

    size_t file_size = fileSize(filename);

    if (!file_size)
        throw invalid_argument("jASTERIX called with empty file '" + filename + "'");

    if (debug_)
        loginf << "jASTERIX: file " << filename << " size " << file_size << logendl;

    traced_assert(!file_.is_open());

    file_.open(filename, file_size);

    if (!file_.is_open())
        throw runtime_error("jASTERIX unable to map file '" + filename + "'");

    return file_size;
}

nlohmann::json jASTERIX::loadFramingDefinition(const std::string& framing_str)
{
    // check framing
    if (!fileExists(definition_path_ + "/framings/" + framing_str + ".json"))
        throw invalid_argument("jASTERIX called with unknown framing '" + framing_str + "'");

    try  // create framing definition
    {
        return json::parse(ifstream(definition_path_ + "/framings/" + framing_str + ".json"));
    }
    catch (json::exception& e)
    {
        throw runtime_error("jASTERIX parsing error in framing definition '" + framing_str +
                            "': " + e.what());
    }
}



std::unique_ptr<nlohmann::json> jASTERIX::runAnalysis(const std::function<void()>& decode,
                                                      unsigned int record_limit)
{
    analysis_.clear();
    skipped_category_counts_.clear();

    // the decode counters accumulate over the instance lifetime, the result reports this call
    size_t frames_base = num_frames_;
    size_t records_base = num_records_;
    size_t ref_errors_base = num_ref_errors_;
    size_t spf_errors_base = num_spf_errors_;

    analysis_mode_ = true;
    analysis_errors_base_ = num_errors_;
    call_record_limit_ = record_limit;

    try
    {
        decode();
    }
    catch (...)
    {
        analysis_mode_ = false;
        call_record_limit_ = 0;
        analysis_.clear();
        skipped_category_counts_.clear();
        throw;
    }

    analysis_mode_ = false;
    call_record_limit_ = 0;

    std::unique_ptr<nlohmann::json> analysis_result {new nlohmann::json()};
    (*analysis_result)["num_frames"] = num_frames_ - frames_base;
    (*analysis_result)["num_records"] = num_records_ - records_base;
    (*analysis_result)["num_errors"] = num_errors_ - analysis_errors_base_;
    (*analysis_result)["num_ref_errors"] = num_ref_errors_ - ref_errors_base;
    (*analysis_result)["num_spf_errors"] = num_spf_errors_ - spf_errors_base;

    addAnalysisResult(*analysis_result);
    addSkippedCategoriesAnalysis(*analysis_result);

    analysis_.clear();
    skipped_category_counts_.clear();

    return analysis_result;
}

void jASTERIX::analyzeFlatChunk(const nlohmann::json& flat_chunk, const nlohmann::json& chunk,
                                bool framing)
{
    // data blocks of categories that were not decoded
    auto count_skipped = [this](const nlohmann::json& data_blocks) {
        if (!data_blocks.is_array())
            return;

        for (const nlohmann::json& data_block : data_blocks)
            if (data_block.contains("category"))
                countSkippedDataBlock(data_block);
    };

    if (framing)
    {
        if (chunk.contains("frames"))
            for (const nlohmann::json& frame : chunk.at("frames"))
                if (frame.contains("content") && frame.at("content").contains("data_blocks"))
                    count_skipped(frame.at("content").at("data_blocks"));
    }
    else if (chunk.contains("data_blocks"))
        count_skipped(chunk.at("data_blocks"));

    // side columns are not data items
    static const std::set<std::string> side_columns{"artas_md5", "record_data", "recording_time",
                                                    "recording_day", "recording_date"};

    const size_t no_sensor = std::numeric_limits<size_t>::max();

    for (const auto& cat_it : flat_chunk.items())
    {
        const std::string& cat_str = cat_it.key();
        const nlohmann::json& columns = cat_it.value();

        if (!columns.is_object())
            continue;

        // records of this category in the chunk: the longest column
        size_t num_records = 0;
        for (const auto& column_it : columns.items())
            if (column_it.value().is_array())
                num_records = std::max(num_records, column_it.value().size());

        if (!num_records)
            continue;

        // data source of every record from the SAC/SIC columns, as index into a small table
        std::vector<std::string> sensors;
        std::vector<size_t> sensor_of_record(num_records, 0);
        {
            const nlohmann::json* sac_col = columns.contains("010.SAC") ? &columns.at("010.SAC") : nullptr;
            const nlohmann::json* sic_col = columns.contains("010.SIC") ? &columns.at("010.SIC") : nullptr;

            std::map<std::pair<long long, long long>, size_t> sensor_index;
            size_t unknown_index = no_sensor;

            for (size_t rec = 0; rec < num_records; ++rec)
            {
                bool known = sac_col && sic_col && rec < sac_col->size() && rec < sic_col->size()
                             && (*sac_col)[rec].is_number() && (*sic_col)[rec].is_number();

                if (!known)
                {
                    if (unknown_index == no_sensor)
                    {
                        unknown_index = sensors.size();
                        sensors.push_back("unknown");
                    }
                    sensor_of_record[rec] = unknown_index;
                    continue;
                }

                std::pair<long long, long long> key{(*sac_col)[rec].get<long long>(),
                                                    (*sic_col)[rec].get<long long>()};
                auto it = sensor_index.find(key);
                if (it == sensor_index.end())
                {
                    it = sensor_index.emplace(key, sensors.size()).first;
                    sensors.push_back(std::to_string(key.first) + "/" + std::to_string(key.second));
                }
                sensor_of_record[rec] = it->second;
            }
        }

        // category statistics per data source, resolved once per chunk
        std::vector<CategoryAnalysis*> cat_stats(sensors.size());
        for (size_t cnt = 0; cnt < sensors.size(); ++cnt)
            cat_stats[cnt] = &analysis_[sensors[cnt]][cat_str];

        for (size_t rec = 0; rec < num_records; ++rec)
            ++cat_stats[sensor_of_record[rec]]->count;

        for (const auto& column_it : columns.items())
        {
            const std::string& path = column_it.key();
            const nlohmann::json& column = column_it.value();

            if (!column.is_array() || side_columns.count(path))
                continue;

            // item statistics per data source, resolved on first use
            std::vector<ItemAnalysis*> item_stats(sensors.size(), nullptr);

            size_t num_cells = std::min(num_records, column.size());
            for (size_t rec = 0; rec < num_cells; ++rec)
            {
                const nlohmann::json& cell = column[rec];
                if (cell.is_null())
                    continue;

                ItemAnalysis*& stats = item_stats[sensor_of_record[rec]];
                if (!stats)
                    stats = &cat_stats[sensor_of_record[rec]]->items[path];

                ++stats->count;

                // bounds for scalar values, arrays (repetitive and extendable items) are counted
                if (cell.is_primitive())
                {
                    if (!stats->has_bounds)
                    {
                        stats->min = cell;
                        stats->max = cell;
                        stats->has_bounds = true;
                    }
                    else
                    {
                        if (cell < stats->min)
                            stats->min = cell;
                        if (cell > stats->max)
                            stats->max = cell;
                    }
                }
            }
        }
    }
}

void jASTERIX::addAnalysisResult(nlohmann::json& analysis_result)
{
    for (const auto& sensor_it : analysis_)
    {
        nlohmann::json& sensor_json = analysis_result[sensor_it.first];

        for (const auto& cat_it : sensor_it.second)
        {
            nlohmann::json& cat_json = sensor_json[cat_it.first];
            cat_json["count"] = cat_it.second.count;

            for (const auto& item_it : cat_it.second.items)
            {
                nlohmann::json& item_json = cat_json[item_it.first];
                item_json["count"] = item_it.second.count;

                if (item_it.second.has_bounds)
                {
                    item_json["min"] = item_it.second.min;
                    item_json["max"] = item_it.second.max;
                }
            }
        }
    }
}

bool jASTERIX::stopAfterChunk() const
{
    // an analysis stops after the first chunk with decode errors, as the record based
    // analysis did
    return recordLimitReached() || (analysis_mode_ && num_errors_ > analysis_errors_base_);
}

void jASTERIX::countSkippedDataBlock(const nlohmann::json& data_block)
{
    unsigned int category = data_block.at("category");

    // only count data blocks skipped because the category cannot be decoded,
    // either since no definition exists or since decoding is disabled
    if (category_definitions_.count(category) && category_definitions_.at(category)->decode())
        return;

    size_t bytes = 0;

    if (data_block.contains("length")) // whole data block incl. CAT/LEN header
        bytes = data_block.at("length");
    else if (data_block.contains("content") && data_block.at("content").contains("length"))
        bytes = data_block.at("content").at("length");

    auto& counts = skipped_category_counts_[category];
    counts.first += 1;
    counts.second += bytes;
}

void jASTERIX::addSkippedCategoriesAnalysis(nlohmann::json& analysis_result)
{
    if (skipped_category_counts_.empty())
        return;

    nlohmann::json& skipped = analysis_result["skipped_categories"];

    for (const auto& cat_it : skipped_category_counts_)
    {
        string cat_str = to_string(cat_it.first);

        skipped[cat_str]["data_blocks"] = cat_it.second.first;
        skipped[cat_str]["bytes"]       = cat_it.second.second;
        skipped[cat_str]["reason"]      = category_definitions_.count(cat_it.first) ?
                    "decoding disabled" : "no specification";
    }
}


void jASTERIX::clearDataChunks()
{
    {
        std::lock_guard<std::mutex> lock(data_chunks_mutex_);
        data_chunks_.clear();
    }
    data_chunks_cv_.notify_one();
}

void jASTERIX::clearDataBlockChunks()
{
    {
        std::lock_guard<std::mutex> lock(data_block_chunks_mutex_);
        data_block_chunks_.clear();
    }
    data_block_chunks_cv_.notify_one();
}

void jASTERIX::resetChunkState()
{
    // a completed producer task of an earlier call leaves the done flag set, so the consumer
    // loop of the next call would end before the new task pushes its first chunk. a stopped
    // task may have left a chunk whose indices point into the buffer of that earlier call.
    clearDataChunks();
    clearDataBlockChunks();

    {
        std::lock_guard<std::mutex> lock(data_chunks_mutex_);
        data_processing_done_ = false;
    }
    {
        std::lock_guard<std::mutex> lock(data_block_chunks_mutex_);
        data_block_processing_done_ = false;
    }
}



void jASTERIX::forceStopTask (DataBlockFinderTask& task)
{
    loginf << "jASTERIX: forceStopTask: data block finder task" << logendl;

    task.forceStop();

    while (!task.done())
    {
        clearDataBlockChunks();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // the task may have pushed a last chunk between the final clear and setting done. it
    // would be decoded against the buffer of the next call.
    clearDataBlockChunks();

    loginf << "jASTERIX: forceStopTask: done" << logendl;
}

void jASTERIX::forceStopTask (FrameParserTask& task)
{
    loginf << "jASTERIX: forceStopTask: frame task" << logendl;

    task.forceStop();

    while (!task.done())
    {
        clearDataChunks();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // same as for the data block finder task: a chunk pushed after the final clear
    clearDataChunks();

    loginf << "jASTERIX: forceStopTask: done" << logendl;
}

std::vector<char> jASTERIX::encodeRecord(unsigned int category,
                                          const nlohmann::json& record_json,
                                          bool debug)
{
    if (category_definitions_.count(category) == 0)
        throw runtime_error("jASTERIX: encodeRecord: category " + to_string(category) + " not defined");

    auto& cat = category_definitions_.at(category);
    auto edition = cat->getCurrentEdition();

    if (!edition)
        throw runtime_error("jASTERIX: encodeRecord: no current edition for category " + to_string(category));

    auto rec = edition->record();

    if (!rec)
        throw runtime_error("jASTERIX: encodeRecord: no record for category " + to_string(category));

    // inject current REF/SPF so they encode also in encode-only runs
    // (otherwise this only happens when an ASTERIXParser is constructed for decoding)
    if (cat->hasCurrentREFEdition())
        rec->setRef(cat->getCurrentREFEdition()->reservedExpansionField());
    if (cat->hasCurrentSPFEdition())
        rec->setSpf(cat->getCurrentSPFEdition()->specialPurposeField());

    // allocate working buffer (64KB should be more than enough for any single record)
    const size_t buf_size = 65536;
    vector<char> buffer(buf_size, 0);

    // encode record starting at offset 3 (leaving room for CAT + LEN)
    size_t record_bytes = rec->encodeRecord(record_json, buffer.data() + 3, buf_size - 3, debug);

    // write CAT byte
    buffer[0] = static_cast<char>(category);

    // write LEN (2 bytes big-endian) = 3 + record_bytes
    unsigned int len = 3 + static_cast<unsigned int>(record_bytes);
    buffer[1] = static_cast<char>((len >> 8) & 0xFF);
    buffer[2] = static_cast<char>(len & 0xFF);

    buffer.resize(len);
    return buffer;
}

std::vector<char> jASTERIX::encodeDataBlock(unsigned int category,
                                             const std::vector<nlohmann::json>& records,
                                             bool debug)
{
    if (category_definitions_.count(category) == 0)
        throw runtime_error("jASTERIX: encodeDataBlock: category " + to_string(category) + " not defined");

    auto& cat = category_definitions_.at(category);
    auto edition = cat->getCurrentEdition();

    if (!edition)
        throw runtime_error("jASTERIX: encodeDataBlock: no current edition for category " + to_string(category));

    auto rec = edition->record();

    if (!rec)
        throw runtime_error("jASTERIX: encodeDataBlock: no record for category " + to_string(category));

    // inject current REF/SPF so they encode also in encode-only runs
    // (otherwise this only happens when an ASTERIXParser is constructed for decoding)
    if (cat->hasCurrentREFEdition())
        rec->setRef(cat->getCurrentREFEdition()->reservedExpansionField());
    if (cat->hasCurrentSPFEdition())
        rec->setSpf(cat->getCurrentSPFEdition()->specialPurposeField());

    // allocate working buffer
    const size_t buf_size = 65536 * records.size();
    vector<char> buffer(buf_size, 0);

    size_t offset = 3; // skip CAT + LEN header

    for (const auto& record_json : records)
    {
        size_t record_bytes = rec->encodeRecord(record_json, buffer.data() + offset,
                                                 buf_size - offset, debug);
        offset += record_bytes;
    }

    // write CAT byte
    buffer[0] = static_cast<char>(category);

    // write LEN (2 bytes big-endian)
    unsigned int len = static_cast<unsigned int>(offset);
    buffer[1] = static_cast<char>((len >> 8) & 0xFF);
    buffer[2] = static_cast<char>(len & 0xFF);

    buffer.resize(len);
    return buffer;
}

}  // namespace jASTERIX
