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
    size_t file_size = openFile(filename);

    const char* data = file_.data();

    nlohmann::json framing_definition = loadFramingDefinition(framing_str);

            // create ASTERIX parser
    ASTERIXParser asterix_parser(data_block_definition_, category_definitions_, debug_);

            // create frame parser
    bool debug_framing = debug_ && !debug_exclude_framing_;
    FrameParser frame_parser(framing_definition, asterix_parser, debug_framing);

    nlohmann::json json_header;

    size_t index{0};

            // parsing header
    if (frame_parser.hasFileHeaderItems())
        index = frame_parser.parseHeader(data, 0, file_size, json_header, debug_framing);

    if (debug_)
        loginf << "jasterix: analyze creating frame parser task index " << index << " header '"
               << json_header.dump(4) << "'" << logendl;

    stop_decoding_ = false;
    std::unique_ptr<nlohmann::json> analysis_result {new nlohmann::json()};
    // ensure required keys always exist, even if the producer task fails before
    // any chunk is decoded
    (*analysis_result)["num_frames"]  = 0;
    (*analysis_result)["num_records"] = 0;
    (*analysis_result)["num_errors"]  = 0;

    std::unique_ptr<FrameParserTask> task {
                                          new FrameParserTask(*this, frame_parser, json_header, data, index, file_size, debug_framing)};
    task->start();

    if (debug_)
    {
        while (!task->done())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::unique_ptr<nlohmann::json> data_chunk;

    size_t num_callback_frames;
    std::pair<size_t, size_t> dec_ret{0, 0};

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

            //loginf << "jASTERIX: analyze frame chunks " << data_chunks_.size() << logendl;
            //  mostly 2. mostly.

            data_chunk = std::move(data_chunks_.front().first);
            data_chunks_.pop_front();
        }
        data_chunks_cv_.notify_one();  // wake producer from backpressure

        if (debug_)
            loginf << "jASTERIX: analyze decoding frames" << logendl;

        num_callback_frames = data_chunk->at("frames").size();
        num_frames_ += num_callback_frames;

        (*analysis_result)["num_frames"] = num_frames_;

        try
        {
            dec_ret = frame_parser.decodeFrames(data, file_size, data_chunk.get(), debug_);
            num_records_ += dec_ret.first;
            num_errors_ += dec_ret.second;

            (*analysis_result)["num_records"] = num_records_;
            (*analysis_result)["num_errors"] = num_errors_;

            if (debug_)
                loginf << "jASTERIX analyze " << num_frames_ << " frames, " << num_records_
                       << " records " << num_errors_ << " errors " << logendl;

            if (num_errors_)
            {
                loginf << "jASTERIX analyze resulted in " << num_errors_ << " errors " << logendl;

                forceStopTask(*task);

                break;
            }

            analyzeChunk(data_chunk, true);

            data_chunk = nullptr;

            if (record_limit > 0 && num_records_ >= record_limit)
            {
                if (debug_)
                    loginf << "jASTERIX analyze hit record limit" << logendl;

                forceStopTask(*task);

                break;
            }
        }
        catch (std::exception& e)
        {
            logerr << "jASTERIX caught exception '" << e.what() << "', breaking" << logendl;

            forceStopTask(*task);

            throw (e);  // rethrow
        }
    }

    if (task->errorOcurred())
    {
        unsigned int num_errors {0};
        if (analysis_result->contains("num_errors"))
            num_errors = analysis_result->at("num_errors");
        ++num_errors;
        (*analysis_result)["num_errors"] = num_errors;
        (*analysis_result)["num_frame_errors"] = 1;
    }

    if (!task->done()) // aborted
        forceStopTask(*task);

    if (debug_)
        loginf << "jASTERIX analyze file done" << logendl;

    file_.close();

            //(*analysis_result)["sensor_counts"] = sensor_counts_;

            //(*analysis_result) = data_item_analysis_;

    for (const auto& ana_it : data_item_analysis_) // add to preserve num counters
        (*analysis_result)[ana_it.first] = ana_it.second;

            //sensor_counts_.clear();
    data_item_analysis_.clear();

    return analysis_result;
}

std::unique_ptr<nlohmann::json> jASTERIX::analyzeFile(const std::string& filename, unsigned int record_limit)
{
    loginf << "jASTERIX: analyzeFile: filename '" << filename << "' record_limit " << record_limit << logendl;

    size_t file_size = openFile(filename);

    const char* data = file_.data();

    return analyzeData(data, file_size, record_limit);
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

    std::map<std::string, std::map<std::string, std::map<std::string, nlohmann::json>>> data_item_analysis =
        *analysis_result;

    ss << toCSV(data_item_analysis);

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

    std::map<std::string, std::map<std::string, std::map<std::string, nlohmann::json>>> data_item_analysis =
        *analysis_result;

    //loginf << "jASTERIX: analyzeFileCSV: toCSV";
    ss << toCSV(data_item_analysis);

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

        std::map<std::string, std::map<std::string, std::map<std::string, nlohmann::json>>>
            data_item_analysis = stream_result;

        ss << toCSV(data_item_analysis);
        ss << endl;
    }

    return ss.str();
}


std::unique_ptr<nlohmann::json> jASTERIX::analyzeData(const char* data, unsigned int total_size,
                                                      unsigned int record_limit)
{
    // create ASTERIX parser
    ASTERIXParser asterix_parser(data_block_definition_, category_definitions_, debug_);

    if (debug_)
        loginf << "jASTERIX: finding data blocks" << logendl;

    size_t index{0};

    std::unique_ptr<DataBlockFinderTask> task {
                                              new DataBlockFinderTask(*this, asterix_parser, data, index, total_size, debug_)};

    task->start();

    if (debug_)
        while (!task->done())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

    std::unique_ptr<nlohmann::json> data_block_chunk;
    std::unique_ptr<nlohmann::json> analysis_result {new nlohmann::json()};
    // ensure required keys always exist, even if the producer task fails before
    // any chunk is decoded
    (*analysis_result)["num_records"] = 0;
    (*analysis_result)["num_errors"]  = 0;

    std::pair<size_t, size_t> dec_ret{0, 0};

    stop_decoding_ = false;

    while (!stop_decoding_)
    {
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

            data_block_chunk = std::move(data_block_chunks_.front().first);
            data_block_chunks_.pop_front();
        }
        data_block_chunks_cv_.notify_one();  // wake producer from backpressure

        if (debug_)
            loginf << "jasterix: analyze decoding data block" << logendl;

        try
        {
            if (!data_block_chunk->contains("data_blocks"))
                throw runtime_error("jasterix data blocks not found");

            if (!data_block_chunk->at("data_blocks").is_array())
                throw runtime_error("jasterix data blocks is not an array");

            dec_ret =
                asterix_parser.decodeDataBlocks(data, total_size, data_block_chunk->at("data_blocks"), debug_);

            //            loginf << "jASTERIX: analyzeFile: decode data block done, num_records " << dec_ret.first
            //                   << " errors " << dec_ret.second << logendl;

            num_records_ += dec_ret.first;
            num_errors_ += dec_ret.second;

            (*analysis_result)["num_records"] = num_records_;
            (*analysis_result)["num_errors"] = num_errors_;

            if (num_errors_)
            {
                loginf << "jASTERIX analyze resulted in " << num_errors_ << " errors " << logendl;

                forceStopTask(*task);

                break;
            }

            analyzeChunk(data_block_chunk, false);

            data_block_chunk = nullptr;

            if (record_limit > 0 && num_records_ >= record_limit)
            {
                if (debug_)
                    loginf << "jASTERIX analyze hit record limit" << logendl;

                forceStopTask(*task);

                break;
            }
        }
        catch (std::exception& e)
        {
            logerr << "jASTERIX caught exception'" << e.what() << "', breaking" << logendl;

            forceStopTask(*task);

            data_item_analysis_.clear();

            throw;
        }
    }

            //loginf << "jASTERIX analyze done with while" << logendl;

    if (task->error())
    {
        unsigned int num_errors {0};

        if (analysis_result->contains("num_errors"))
            num_errors = analysis_result->at("num_errors");

        ++num_errors;

        (*analysis_result)["num_errors"] = num_errors;
    }

            //loginf << "jASTERIX analyze waiting on task force stop" << logendl;

    if (!task->done()) // aborted
        forceStopTask(*task);

    if (debug_)
        loginf << "jASTERIX decode file done" << logendl;

    file_.close();

            //(*analysis_result)["sensor_counts"] = sensor_counts_;

            //if (csv)

    //(*analysis_result) = data_item_analysis_;

    for (const auto& ana_it : data_item_analysis_) // add to preserve num counters
        (*analysis_result)[ana_it.first] = ana_it.second;

            //sensor_counts_.clear();
    data_item_analysis_.clear();

    return analysis_result;
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

    std::map<std::string, std::map<std::string, std::map<std::string, nlohmann::json>>> data_item_analysis =
        *analysis_result;

    ss << toCSV(data_item_analysis);

    return ss.str();
}

void jASTERIX::setupFlatColumns()
{
    flat_data_.clear();
    flat_hash_columns_.clear();

    for (auto& [cat, cat_def] : category_definitions_)
    {
        if (cat_def->decode())
        {
            flat_record_indices_[cat] = 0;
            cat_def->setupColumnWriters([this, cat](ItemParserBase* leaf, const std::string& name) {
                flat_data_[cat][name] = nlohmann::json::array();
                leaf->setColumnTarget(&flat_data_[cat][name], &flat_record_indices_[cat]);
            });

#if USE_OPENSSL
            if (add_artas_md5_hash)
            {
                flat_data_[cat]["artas_md5"] = nlohmann::json::array();
                flat_hash_columns_[cat] = &flat_data_[cat]["artas_md5"];
            }
#endif
        }
    }
}

std::unique_ptr<nlohmann::json> jASTERIX::moveFlatData()
{
    auto result = std::make_unique<nlohmann::json>();
    for (auto& [cat, cat_data] : flat_data_)
    {
        auto idx_it = flat_record_indices_.find(cat);
        if (idx_it == flat_record_indices_.end() || idx_it->second == 0)
            continue;

        nlohmann::json filtered_cat = nlohmann::json::object();
        for (auto it = cat_data.begin(); it != cat_data.end(); ++it)
        {
            if (it.value().is_array() && it.value().empty())
                continue;
            filtered_cat[it.key()] = std::move(it.value());
        }
        (*result)[std::to_string(cat)] = std::move(filtered_cat);
    }

    setupFlatColumns();  // re-create fresh arrays and re-inject pointers

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

            // create ASTERIX parser
    ASTERIXParser asterix_parser(data_block_definition_, category_definitions_, debug_);

    if (do_flat)
    {
        flat_record_indices_.clear();
        setupFlatColumns();
        asterix_parser.setFlatRecordIndices(&flat_record_indices_);
        asterix_parser.setFlatHashColumns(&flat_hash_columns_);
        asterix_parser.setFlatData(&flat_data_);
    }

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

            if (debug_)
                loginf << "jASTERIX processing " << num_frames_ << " frames, " << num_records_
                       << " records " << num_errors_ << " errors " << logendl;

            if (do_flat)
            {
                auto flat_chunk = moveFlatData();

                if (print_)
                    std::cout << flat_chunk->dump(print_dump_indent) << std::endl;

                if (data_callback)
                    data_callback(std::move(flat_chunk), chunk_bytes_read, num_callback_frames, dec_ret.first, dec_ret.second);

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

    // create ASTERIX parser
    ASTERIXParser asterix_parser(data_block_definition_, category_definitions_, debug_);

    if (do_flat)
    {
        flat_record_indices_.clear();
        setupFlatColumns();
        asterix_parser.setFlatRecordIndices(&flat_record_indices_);
        asterix_parser.setFlatHashColumns(&flat_hash_columns_);
        asterix_parser.setFlatData(&flat_data_);
    }

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

            if (do_flat)
            {
                auto flat_chunk = moveFlatData();

                if (print_)
                    std::cout << flat_chunk->dump(print_dump_indent) << std::endl;

                if (data_callback)
                    data_callback(std::move(flat_chunk), chunk_bytes_read, 0, dec_ret.first, dec_ret.second);

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
    ASTERIXParser asterix_parser_instance (data_block_definition_, category_definitions_, debug_);

    if (do_flat)
    {
        flat_record_indices_.clear();
        setupFlatColumns();
        asterix_parser_instance.setFlatRecordIndices(&flat_record_indices_);
        asterix_parser_instance.setFlatHashColumns(&flat_hash_columns_);
    }
    else
    {
        asterix_parser_instance.setFlatRecordIndices(nullptr);
        asterix_parser_instance.setFlatHashColumns(nullptr);
        asterix_parser_instance.setFlatData(nullptr);
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

            dec_ret =
                asterix_parser_instance.decodeDataBlocks(data, total_size, data_block_chunk->at("data_blocks"), debug_);
            num_records_ += dec_ret.first;
            num_errors_ += dec_ret.second;

            // when decoding a PCAP, stamp each data block with its network capture time
            // (before printing / callback so both see it). only for structured output.
            if (pcap_packet_times_ && !do_flat)
                stampPCAPTimes(data_block_chunk->at("data_blocks"));

            if (do_flat)
            {
                auto flat_chunk = moveFlatData();

                if (print_)
                    std::cout << flat_chunk->dump(print_dump_indent) << std::endl;

                if (data_callback)
                    data_callback(std::move(flat_chunk), chunk_bytes_read, 0, dec_ret.first, dec_ret.second);

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

void jASTERIX::stampPCAPTimes(nlohmann::json& data_blocks)
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

        data_block["pcap_time"]       = PcapReader::timeToString(ts);
        data_block["pcap_time_epoch"] = ts;
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

    stop_decoding_ = false;

    std::vector<char> chunk;
    bool              eof = false;

    while (!eof && !stop_decoding_)
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

void jASTERIX::analyzeChunk(const std::unique_ptr<nlohmann::json>& data_chunk, bool framing)
{
    unsigned int category;

    if (framing)
    {
        if (!data_chunk->contains("frames"))
            return;

        traced_assert(data_chunk->contains("frames"));

        for (const auto& frame : data_chunk->at("frames"))
        {
            if (!frame.contains("content") || !frame.at("content").contains("data_blocks"))
                continue;

            traced_assert(frame.contains("content"));
            traced_assert(frame.at("content").contains("data_blocks"));

            for (const auto& data_block : frame.at("content").at("data_blocks"))
            {
                traced_assert(data_block.contains("category"));

                if (!data_block.contains("content") || !data_block.at("content").contains("records"))
                    continue;

                traced_assert(data_block.contains("content"));
                traced_assert(data_block.at("content").contains("records"));

                category = data_block.at("category");

                for (const auto& record : data_block.at("content").at("records"))
                {
                    analyzeRecord (category, record);
                }
            }
        }
    }
    else // no framing
    {
        if (!data_chunk->contains("data_blocks"))
            return;

        traced_assert(data_chunk->contains("data_blocks"));

        for (const auto& data_block : data_chunk->at("data_blocks"))
        {
            traced_assert(data_block.contains("category"));

            if (!data_block.contains("content") || !data_block.at("content").contains("records"))
                continue;

            traced_assert(data_block.contains("content"));
            traced_assert(data_block.at("content").contains("records"));

            category = data_block.at("category");

            for (const auto& record : data_block.at("content").at("records"))
            {
                analyzeRecord (category, record);
            }
        }
    }
}

void jASTERIX::analyzeRecord(unsigned int category, const nlohmann::json& record)
{
    string cat_str = to_string(category);

            // sensor

    string sensor_id;

    if (record.contains("010") && record.at("010").count("SAC") && record.at("010").count("SIC"))
        sensor_id = to_string(record.at("010").at("SAC")) + "/" + to_string(record.at("010").at("SIC"));
    else
        sensor_id = "unknown";

            //data_item_analysis_[sensor_id][cat_str] += 1;

            // data item analysis
    if (data_item_analysis_.count(sensor_id)
        && data_item_analysis_.at(sensor_id).count(cat_str)
        && data_item_analysis_.at(sensor_id).at(cat_str).count("count"))
    {
        unsigned int count = data_item_analysis_.at(sensor_id).at(cat_str).at("count");
        data_item_analysis_[sensor_id][cat_str]["count"] = count + 1;
    }
    else
        data_item_analysis_[sensor_id][cat_str]["count"] = 1;

    traced_assert(record.is_object());

    addJSONAnalysis(sensor_id, cat_str, "", record);
}


void jASTERIX::addJSONAnalysis(const std::string& sensor_id, const std::string& cat_str,
                               const std::string& prefix, const nlohmann::json& item)
{
    traced_assert(item.is_object());

    string sub_prefix;
    bool is_primitive;

    for (const auto& item_it : item.items())
    {
        if (prefix.size())
            sub_prefix = prefix+"."+item_it.key();
        else
            sub_prefix = item_it.key();

        if (item_it.value().is_object())
            addJSONAnalysis(sensor_id, cat_str, sub_prefix, item_it.value());
        else
        {
            is_primitive = item_it.value().is_primitive();

            // navigate to the inner entry using find() to avoid repeated lookups
            auto sit = data_item_analysis_.find(sensor_id);
            if (sit != data_item_analysis_.end())
            {
                auto cit = sit->second.find(cat_str);
                if (cit != sit->second.end())
                {
                    auto pit = cit->second.find(sub_prefix);
                    if (pit != cit->second.end())
                    {
                        auto& entry = pit->second;  // the count/min/max json object

                        unsigned int count = entry.at("count");
                        entry["count"] = count + 1;

                        if (is_primitive)
                        {
                            entry["min"] = min(item_it.value(), entry.at("min"));
                            entry["max"] = max(item_it.value(), entry.at("max"));
                        }

                        continue;  // done with this item
                    }
                }
            }

            // first occurrence — create entry via operator[]
            {
                auto& entry = data_item_analysis_[sensor_id][cat_str][sub_prefix];
                entry["count"] = 1;

                if (is_primitive)
                {
                    entry["min"] = item_it.value();
                    entry["max"] = item_it.value();
                }
            }
        }
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


std::string jASTERIX::toCSV (
    const std::map<std::string, std::map<std::string, std::map<std::string, nlohmann::json>>>& data_item_analysis)
{
    // sac/sic -> cat -> key -> count/min/max

    std::stringstream ss;

    ss << "sac/sic;name;count;min;max" << endl;

    string cat_str;

    for (const auto& sensor_it : data_item_analysis)
    {
        loginf << sensor_it.first << sensor_it.second;

        for (const auto& cat_it : sensor_it.second)
        {
            std::ostringstream oss;

            // Format the number: width=3, fill='0'
            oss << std::setw(3) << std::setfill('0') << cat_it.first;

            cat_str = oss.str();

            ss << sensor_it.first << " CAT" << cat_str << ":" << endl;

            for (const auto& di_info_it : cat_it.second) // key -> count/min/max
            {
                if (di_info_it.second.is_primitive())
                {
                    ss << sensor_it.first << ";count;" << di_info_it.second << ";;" << endl;

                    continue;
                }

                ss << sensor_it.first << ";" << di_info_it.first << ";" << di_info_it.second.at("count");
                ss << ";";

                if (di_info_it.second.contains("min"))
                    ss << di_info_it.second.at("min");

                ss << ";";

                if (di_info_it.second.contains("max"))
                    ss << di_info_it.second.at("max");

                ss << endl;
            }

            ss << endl << endl;
        }
    }

    return ss.str();
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
