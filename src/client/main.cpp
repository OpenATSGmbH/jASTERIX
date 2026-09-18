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
#include "jasterix/global.h"
#include "jsonwriter.h"
#include "logger.h"
#include "string_conv.h"
#include "traced_assert.h"

#if USE_OPENSSL
#include "utils/hashchecker.h"
#endif

#include <boost/program_options.hpp>

#include "boost/date_time/posix_time/posix_time.hpp"

namespace po = boost::program_options;

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>
#include <thread>
#include <memory>

#include <archive.h>
#include <archive_entry.h>

#include <tbb/tbb.h>

#if USE_LOG4CPP
#include "log4cpp/Layout.hh"
#include "log4cpp/OstreamAppender.hh"
#include "log4cpp/SimpleLayout.hh"
#endif

using namespace std;

extern std::unique_ptr<jASTERIX::JSONWriter> json_writer;

std::unique_ptr<jASTERIX::JSONWriter> json_writer;

namespace
{
// Flat encode helpers: turn the flat columnar JSON (cat -> leaf_name -> array)
// produced by --flat back into raw/netto ASTERIX via the jASTERIX encode API.

// Set a value at a dotted leaf path ("080.CST") inside a nested record object.
void encodeSetPath(nlohmann::json& obj, const std::string& path, const nlohmann::json& val)
{
    nlohmann::json* cur = &obj;
    size_t start = 0;
    while (true)
    {
        size_t dot = path.find('.', start);
        if (dot == std::string::npos)
        {
            (*cur)[path.substr(start)] = val;
            break;
        }
        cur = &((*cur)[path.substr(start, dot - start)]);
        start = dot + 1;
    }
}

// Side columns written next to the item leaves in flat mode. They are not record content
// and must not be encoded.
bool encodeIsSideColumn(const std::string& name)
{
    return name == "artas_md5" || name == "record_data" || name == "recording_time"
           || name == "recording_day" || name == "recording_date";
}

// Reconstruct nested per-record JSON objects from one category's flat columns.
// A null column entry means the item/subfield was not present in that record.
// Repetitive item leaves are flattened as struct-of-arrays: the cell is an array
// of scalars aligned by repetition index (e.g. 'SPF.Target Report Identifiers.TRI'
// -> ["c1f176d0", ...]). Those are zipped back into the array-of-objects form the
// nested encoder expects at the parent path. Extendable item cells are arrays of
// objects and pass through unchanged.
std::vector<nlohmann::json> encodeReconstructRecords(const nlohmann::json& cat_cols)
{
    size_t num_records = 0;
    for (auto it = cat_cols.begin(); it != cat_cols.end(); ++it)
        if (it.value().is_array())
            num_records = std::max(num_records, it.value().size());

    std::vector<nlohmann::json> records;
    records.reserve(num_records);

    for (size_t i = 0; i < num_records; ++i)
    {
        nlohmann::json rec = nlohmann::json::object();
        for (auto it = cat_cols.begin(); it != cat_cols.end(); ++it)
        {
            if (encodeIsSideColumn(it.key()))
                continue;

            const nlohmann::json& col = it.value();
            if (!col.is_array() || i >= col.size())
                continue;
            const nlohmann::json& val = col.at(i);
            if (val.is_null())
                continue;

            const std::string& path = it.key();
            size_t last_dot = path.rfind('.');

            if (val.is_array() && last_dot != std::string::npos &&
                (val.empty() || !val.at(0).is_object()))
            {
                // repetitive leaf: zip scalars into array-of-objects at parent path
                nlohmann::json* parent = &rec;
                size_t start = 0;
                while (true)
                {
                    size_t dot = path.find('.', start);
                    if (dot == std::string::npos || dot == last_dot)
                        break;
                    parent = &((*parent)[path.substr(start, dot - start)]);
                    start = dot + 1;
                }
                nlohmann::json& container = (*parent)[path.substr(start, last_dot - start)];
                const std::string leaf = path.substr(last_dot + 1);

                for (size_t k = 0; k < val.size(); ++k)
                    container[k][leaf] = val.at(k);
            }
            else
                encodeSetPath(rec, path, val);
        }
        records.push_back(std::move(rec));
    }
    return records;
}

// Encode one flat chunk object (top-level cat -> columns) into data blocks and
// append them to out. Records are batched into data blocks bounded by the 2-byte
// ASTERIX length field (max 65535 bytes per block).
void encodeFlatChunk(jASTERIX::jASTERIX& asterix, const nlohmann::json& chunk, int only_cat,
                     std::ofstream& out, size_t& total_records, size_t& total_blocks)
{
    for (auto it = chunk.begin(); it != chunk.end(); ++it)
    {
        const std::string& key = it.key();

        // category keys are numeric strings (e.g. "62"); skip "rec_num" etc.
        if (key.empty() ||
            !std::all_of(key.begin(), key.end(), [](unsigned char c) { return std::isdigit(c); }))
            continue;
        if (!it.value().is_object())
            continue;

        unsigned int cat = static_cast<unsigned int>(std::atoi(key.c_str()));
        if (only_cat > 0 && cat != static_cast<unsigned int>(only_cat))
            continue;

        std::vector<nlohmann::json> records = encodeReconstructRecords(it.value());
        if (records.empty())
            continue;

        const size_t max_data_block = 65535;
        std::vector<nlohmann::json> group;
        size_t group_payload = 0;

        auto flush = [&]() {
            if (group.empty())
                return;
            std::vector<char> bytes = asterix.encodeDataBlock(cat, group);
            out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
            ++total_blocks;
            group.clear();
            group_payload = 0;
        };

        for (const auto& rec : records)
        {
            // measure the record's encoded payload (encodeRecord returns CAT+LEN+record)
            std::vector<char> one = asterix.encodeRecord(cat, rec);
            size_t payload = one.size() - 3;

            if (!group.empty() && 3 + group_payload + payload > max_data_block)
                flush();

            group.push_back(rec);
            group_payload += payload;
            ++total_records;
        }
        flush();
    }
}

// Parse newline-separated flat chunk objects (one compact object per line, as
// written by --flat --print_indent -1) and encode each.
void encodeFlatText(jASTERIX::jASTERIX& asterix, const std::string& text, int only_cat,
                    std::ofstream& out, size_t& total_records, size_t& total_blocks,
                    size_t& total_chunks)
{
    size_t start = 0;
    while (start < text.size())
    {
        size_t nl = text.find('\n', start);
        std::string line =
            text.substr(start, nl == std::string::npos ? std::string::npos : nl - start);
        start = (nl == std::string::npos) ? text.size() : nl + 1;

        size_t b = line.find_first_not_of(" \t\r\n");
        if (b == std::string::npos)
            continue;
        size_t e = line.find_last_not_of(" \t\r\n");
        line = line.substr(b, e - b + 1);
        if (line.empty())
            continue;

        try
        {
            nlohmann::json chunk = nlohmann::json::parse(line);
            encodeFlatChunk(asterix, chunk, only_cat, out, total_records, total_blocks);
            ++total_chunks;
        }
        catch (std::exception& ex)
        {
            logerr << "jASTERIX client: failed to parse flat chunk: " << ex.what() << logendl;
        }
    }
}

// Read the full content of the current archive entry.
std::string encodeReadArchiveEntry(struct archive* a)
{
    std::string content;
    const void* buff;
    size_t size;
    la_int64_t offset;

    while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK)
        content.append(static_cast<const char*>(buff), size);

    return content;
}
}  // namespace

void write_callback(std::unique_ptr<nlohmann::json> data_chunk, size_t total_num_bytes,
                    size_t num_frames, size_t num_records, size_t num_errors)
{
    //    loginf << "jASTERIX: write_callback " << num_frames << " frames, " << num_records << "
    //    records, "
    //           << num_errors << " errors";

    traced_assert(json_writer);
    json_writer->write(std::move(data_chunk));
}

void empty_callback(std::unique_ptr<nlohmann::json> data_chunk, size_t total_num_bytes,
                    size_t num_frames, size_t num_records, size_t num_errors)
{
    traced_assert(data_chunk);
}

int main(int argc, char** argv)
{
    static_assert(sizeof(size_t) >= 8, "code requires size_t with at least 8 bytes");

    // setup logging

#if USE_LOG4CPP
    log4cpp::Appender* console_appender_ = new log4cpp::OstreamAppender("console", &std::cout);
    console_appender_->setLayout(new log4cpp::SimpleLayout());

    log4cpp::Category& root = log4cpp::Category::getRoot();
    root.setPriority(log4cpp::Priority::INFO);
    root.addAppender(console_appender_);
#endif

    //tbb::task_scheduler_init guard(std::thread::hardware_concurrency());

    std::string filename;
    std::string framing{""};
    std::string definition_path;
    std::string only_cats;
    std::string editions;
    std::string ref_editions;
    std::string spf_editions;
    bool pcap{false};
    bool debug{false};
    bool debug_include_framing{false};
    bool print{false};
    bool print_cat_info{false};
    std::string write_type;
    std::string write_filename;
    bool log_performance{false};

    std::string encode_flat_zip;
    std::string encode_flat;
    std::string encode_filename;
    int encode_cat{0};

    bool flat{false};

    bool analyze{false};
    bool analyze_csv{false};
    unsigned int analyze_record_limit {0};

    po::options_description desc("Allowed options");
    desc.add_options()("help", "produce help message")(
                "filename", po::value<std::string>(&filename), "input file name")(
                "definition_path", po::value<std::string>(&definition_path),
                "path to jASTERIX definition files")(
                "framing", po::value<std::string>(&framing),
                "input framine format, as specified in the framing definitions."
                " raw/netto is default")(
                "frame_limit", po::value<int>(&jASTERIX::frame_limit),
                "number of frames to process with framing, default -1, use -1 to disable.")(
                "frame_chunk_size", po::value<int>(&jASTERIX::frame_chunk_size),
                "number of frames to process in one chunk, default 1000, use -1 to disable.")(
                "data_block_limit", po::value<int>(&jASTERIX::data_block_limit),
                "number of data blocks to process without framing, default -1, use -1 to disable.")(
                "data_block_chunk_size", po::value<int>(&jASTERIX::data_block_chunk_size),
                "number of data blocks to process in one chunk, default 1000, use -1 to disable.")(
                "data_write_size", po::value<int>(&jASTERIX::data_write_size),
                "number of frame chunks to write in one file write, default 1, use -1 to disable.")(
                "debug", po::bool_switch(&debug), "print debug output (only for small files)")(
                "debug_include_framing", po::bool_switch(&debug_include_framing),
                "print debug output including framing, debug still has to be set, disable per default")(
                "print_cat_info", po::bool_switch(&print_cat_info), "print category info")(
                "single_thread", po::bool_switch(&jASTERIX::single_thread),
                "process data in single thread")("only_cats", po::value<std::string>(&only_cats),
                                                 "restricts categories to be decoded, e.g. 20,21.")(
                "editions", po::value<std::string>(&editions),
                "set non-default editions per category, e.g. 21:0.26,48:1.15.")(
                "ref_edition", po::value<std::string>(&ref_editions),
                "set non-default REF editions per category, e.g. 21:1.5,62:1.4.")(
                "spf_edition", po::value<std::string>(&spf_editions),
                "set non-default SPF editions per category, e.g. 21:Aireon,62:ARTAS.")(
                "pcap", po::bool_switch(&pcap),
                "input file is a PCAP capture (libpcap); ASTERIX payload is extracted and "
                "decoded as raw/netto (no framing).")(
                "log_perf", po::bool_switch(&log_performance), "enable performance log after processing")(
                "analyze", po::bool_switch(&analyze), "analyze data sources and contents")(
                "analyze_csv", po::bool_switch(&analyze_csv), "analyze data sources and contents, print as CSV")(
                "analyze_record_limit", po::value<unsigned int>(&analyze_record_limit),
                "number of records to analyze. 0 (default) disables limit. same as record_limit, "
                "kept for compatibility.")(
                "record_limit", po::value<int>(&jASTERIX::record_limit),
                "number of records to process, decode and analyze. stops at the end of the chunk "
                "reaching the limit, so slightly more records are delivered. default -1, use -1 "
                "to disable.")
        #if USE_OPENSSL
            ("add_artas_md5", po::bool_switch(&jASTERIX::add_artas_md5_hash), "add ARTAS MD5 hashes")(
                "check_artas_md5", po::value<std::string>(&check_artas_md5_hash),
                "add and check ARTAS MD5 hashes (with record data), stating which categories to check, "
                "e.g. 1,20,21,48")
        #endif
            ("flat", po::bool_switch(&flat),
             "output in flat/columnar format (cat -> leaf_name -> array)")
            ("add_record_data", po::bool_switch(&jASTERIX::add_record_data),
             "add original record data in hex")("print", po::bool_switch(&print),
                                                "print JSON output")(
                "print_indent", po::value<int>(&jASTERIX::print_dump_indent),
                "intendation of json print, use -1 to disable.")(
                "write_type", po::value<std::string>(&write_type),
                "optional write type, e.g. text,zip. needs write_filename.")(
                "write_filename", po::value<std::string>(&write_filename),
                "optional write filename, e.g. test.zip.")(
                "encode_flat_zip", po::value<std::string>(&encode_flat_zip),
                "encode flat columnar JSON from a zip (members = chunks, as written by "
                "--flat --write_type zip) back to raw/netto ASTERIX. needs encode_filename.")(
                "encode_flat", po::value<std::string>(&encode_flat),
                "encode flat columnar JSON from a text file (one flat chunk object per line) "
                "back to raw/netto ASTERIX. needs encode_filename.")(
                "encode_filename", po::value<std::string>(&encode_filename),
                "output binary file for flat encoding (raw/netto ASTERIX).")(
                "encode_cat", po::value<int>(&encode_cat),
                "restrict flat encoding to a single category. 0 (default) encodes all "
                "categories present. editions are taken from --editions (defaults otherwise).");

    try
    {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (!definition_path.size())
        {
            logerr << "mandatory definition path missing, please use the following arguments: " << logendl << logendl;
            
            loginf << desc;
            return 1;
        }        

        if (vm.count("help"))
        {
            loginf << desc;
            return 1;
        }
    }
    catch (exception& e)
    {
        logerr << "jASTERIX client: unable to parse command line parameters: \n"
               << e.what() << logendl;
        return -1;
    }

#if USE_OPENSSL
    if (check_artas_md5_hash.size())
    {
        jASTERIX::add_artas_md5_hash = true;
        jASTERIX::add_record_data = true;

        if (write_type.size())
        {
            logerr << "jASTERIX client: writing can not be used while artas md5 checking"
                   << logendl;
            return -1;
        }

        std::vector<std::string> cat_strs;
        split(check_artas_md5_hash, ',', cat_strs);

        int cat;
        for (auto& cat_str : cat_strs)
        {
            cat = std::atoi(cat_str.c_str());
            if (cat < 1 || cat > 255)
            {
                logerr << "jASTERIX client: impossible artas md5 checking cat value '" << cat_str
                       << "'" << logendl;
                return -1;
            }
            check_artas_md5_categories.push_back(cat);
        }
        if (!check_artas_md5_categories.size())
        {
            logerr << "jASTERIX client: no valid artas md5 checking cat values given" << logendl;
            return -1;
        }

        hash_checker.reset(new HashChecker(framing.size()));  // true if framing set
    }
#endif

    if (write_type.size())
    {
        if (write_type != "text" && write_type != "zip")
        {
            logerr << "jASTERIX client: unknown write_type '" << write_type << "'" << logendl;
            return -1;
        }

        if (!write_filename.size())
        {
            logerr << "jASTERIX client: write_type '" << write_type
                   << "' requires write_filename to be set" << logendl;
            return -1;
        }

        if (write_type == "text")
            json_writer.reset(new jASTERIX::JSONWriter(jASTERIX::JSON_TEXT, write_filename));
        else if (write_type == "zip")
            json_writer.reset(new jASTERIX::JSONWriter(jASTERIX::JSON_ZIP_TEXT, write_filename));
    }

    std::vector<unsigned int> cat_list;
    if (only_cats.size())
    {
        std::vector<std::string> cat_strings;
        split(only_cats, ',', cat_strings);

        int cat;

        for (auto& cat_it : cat_strings)
        {
            cat = std::atoi(cat_it.c_str());
            if (cat < 1 || cat > 255)
            {
                logerr << "jASTERIX client: impossible cat value '" << cat_it << "'" << logendl;
                return -1;
            }
            cat_list.push_back(static_cast<unsigned int>(cat));
        }
    }

    // cat -> edition pairs from "21:0.26,48:1.15"
    std::vector<std::pair<unsigned int, std::string>> edition_list;
    if (editions.size())
    {
        std::vector<std::string> edition_strings;
        split(editions, ',', edition_strings);

        for (auto& ed_it : edition_strings)
        {
            std::vector<std::string> parts;
            split(ed_it, ':', parts);

            if (parts.size() != 2 || !parts[0].size() || !parts[1].size())
            {
                logerr << "jASTERIX client: invalid edition spec '" << ed_it
                       << "', expected cat:edition" << logendl;
                return -1;
            }

            int cat = std::atoi(parts[0].c_str());
            if (cat < 1 || cat > 255)
            {
                logerr << "jASTERIX client: impossible cat value '" << parts[0] << "'" << logendl;
                return -1;
            }

            edition_list.emplace_back(static_cast<unsigned int>(cat), parts[1]);
        }
    }

    // cat -> REF / SPF edition pairs, same syntax as --editions
    auto parse_edition_list = [](const std::string& spec, const std::string& what,
                                 std::vector<std::pair<unsigned int, std::string>>& list) -> bool {
        if (!spec.size())
            return true;

        std::vector<std::string> specs;
        split(spec, ',', specs);

        for (auto& spec_it : specs)
        {
            std::vector<std::string> parts;
            split(spec_it, ':', parts);

            if (parts.size() != 2 || !parts[0].size() || !parts[1].size())
            {
                logerr << "jASTERIX client: invalid " << what << " spec '" << spec_it
                       << "', expected cat:edition" << logendl;
                return false;
            }

            int cat = std::atoi(parts[0].c_str());
            if (cat < 1 || cat > 255)
            {
                logerr << "jASTERIX client: impossible cat value '" << parts[0] << "'" << logendl;
                return false;
            }

            list.emplace_back(static_cast<unsigned int>(cat), parts[1]);
        }

        return true;
    };

    std::vector<std::pair<unsigned int, std::string>> ref_edition_list;
    std::vector<std::pair<unsigned int, std::string>> spf_edition_list;

    if (!parse_edition_list(ref_editions, "REF edition", ref_edition_list)
        || !parse_edition_list(spf_editions, "SPF edition", spf_edition_list))
        return -1;

    // check if basic configuration works
    try
    {
        if (debug)
            loginf << "jASTERIX client: startup with filename '" << filename << "' framing '"
                   << framing << "' definition_path '" << definition_path << "' debug " << debug
                   << logendl;

        jASTERIX::jASTERIX asterix(definition_path, print, debug, !debug_include_framing);

        if (cat_list.size())
        {
            asterix.decodeNoCategories();

            for (auto cat_it : cat_list)
            {
                asterix.setDecodeCategory(cat_it, true);

                if (debug)
                    loginf << "jASTERIX client: decoding category " << cat_it << logendl;
            }
        }

        for (auto& ed_it : edition_list)
        {
            if (!asterix.hasCategory(ed_it.first))
            {
                logerr << "jASTERIX client: edition set for unknown category " << ed_it.first
                       << logendl;
                return -1;
            }

            if (!asterix.category(ed_it.first)->hasEdition(ed_it.second))
            {
                logerr << "jASTERIX client: category " << ed_it.first << " has no edition '"
                       << ed_it.second << "'" << logendl;
                return -1;
            }

            asterix.category(ed_it.first)->setCurrentEdition(ed_it.second);

            loginf << "jASTERIX client: category " << ed_it.first << " using edition '"
                   << ed_it.second << "'" << logendl;
        }

        for (auto& ed_it : ref_edition_list)
        {
            if (!asterix.hasCategory(ed_it.first)
                || !asterix.category(ed_it.first)->hasREFEdition(ed_it.second))
            {
                logerr << "jASTERIX client: category " << ed_it.first << " has no REF edition '"
                       << ed_it.second << "'" << logendl;
                return -1;
            }

            asterix.category(ed_it.first)->setCurrentREFEdition(ed_it.second);

            loginf << "jASTERIX client: category " << ed_it.first << " using REF edition '"
                   << ed_it.second << "'" << logendl;
        }

        for (auto& ed_it : spf_edition_list)
        {
            if (!asterix.hasCategory(ed_it.first)
                || !asterix.category(ed_it.first)->hasSPFEdition(ed_it.second))
            {
                logerr << "jASTERIX client: category " << ed_it.first << " has no SPF edition '"
                       << ed_it.second << "'" << logendl;
                return -1;
            }

            asterix.category(ed_it.first)->setCurrentSPFEdition(ed_it.second);

            loginf << "jASTERIX client: category " << ed_it.first << " using SPF edition '"
                   << ed_it.second << "'" << logendl;
        }

        if (encode_filename.size())
        {
            if (!encode_flat_zip.size() && !encode_flat.size())
            {
                logerr << "jASTERIX client: encode_filename requires encode_flat_zip or "
                          "encode_flat" << logendl;
                return -1;
            }

            std::ofstream out(encode_filename, std::ios::binary);
            if (!out)
            {
                logerr << "jASTERIX client: cannot open encode output '" << encode_filename << "'"
                       << logendl;
                return -1;
            }

            size_t total_records = 0, total_blocks = 0, total_chunks = 0;

            if (encode_flat_zip.size())
            {
                struct archive* a = archive_read_new();
                archive_read_support_format_zip(a);

                if (archive_read_open_filename(a, encode_flat_zip.c_str(), 65536) != ARCHIVE_OK)
                {
                    logerr << "jASTERIX client: cannot open zip '" << encode_flat_zip
                           << "': " << archive_error_string(a) << logendl;
                    archive_read_free(a);
                    return -1;
                }

                struct archive_entry* entry;
                while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
                {
                    std::string content = encodeReadArchiveEntry(a);
                    encodeFlatText(asterix, content, encode_cat, out, total_records, total_blocks,
                                   total_chunks);
                }
                archive_read_free(a);
            }
            else
            {
                std::ifstream in(encode_flat);
                if (!in)
                {
                    logerr << "jASTERIX client: cannot open '" << encode_flat << "'" << logendl;
                    return -1;
                }
                std::stringstream ss;
                ss << in.rdbuf();
                encodeFlatText(asterix, ss.str(), encode_cat, out, total_records, total_blocks,
                               total_chunks);
            }

            out.close();
            loginf << "jASTERIX client: encoded " << total_records << " records in "
                   << total_blocks << " data blocks from " << total_chunks
                   << " chunks to '" << encode_filename << "'" << logendl;
            return 0;
        }

        if (print_cat_info)
        {

            for (const auto& cat_it : asterix.categories())
            {
                if (cat_it.first != 48)
                    continue;

                jASTERIX::CategoryItemInfo info = cat_it.second->itemInfo();

                loginf << "cat " << cat_it.first << ": " << logendl;

                for (auto& info_it : info)
                {
                    string editions;

                    for (auto& ed_it : info_it.second.editions_)
                    {
                        if (editions.size())
                            editions += ",";

                        editions += ed_it;
                    }

                    string description;
                    description = info_it.second.description_;

                    description.erase(std::remove(description.begin(), description.end(), '\n'), description.end());


                    loginf << "'" << info_it.first << "'; '" << info_it.second.description_ << "';"
                           << editions << logendl;
                }

                loginf << logendl << logendl;
            }

            return 0;
        }

        boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::local_time();

        if (analyze)
        {
            loginf << "jASTERIX client: analyzing, framing '"
                   << framing << "' analyze_record_limit " << analyze_record_limit << logendl;

            string tmp_str;

            if (pcap)
            {
                if (analyze_csv)
                    tmp_str = asterix.analyzePCAPFileCSV(filename, analyze_record_limit);
                else
                    tmp_str = asterix.analyzePCAPFile(filename, analyze_record_limit)->dump(4);
            }
            else if (framing == "netto" || framing == "")
            {
                if (analyze_csv)
                    tmp_str = asterix.analyzeFileCSV(filename, analyze_record_limit);
                else
                    tmp_str = asterix.analyzeFile(filename, analyze_record_limit)->dump(4);
            }
            else
            {
                if (analyze_csv)
                    tmp_str = asterix.analyzeFileCSV(filename, framing, analyze_record_limit);
                else
                    tmp_str = asterix.analyzeFile(filename, framing, analyze_record_limit)->dump(4);
            }

            loginf << "jASTERIX client: analysis result:" << logendl;
            loginf << tmp_str << logendl;
        }
        else
        {
            if (pcap)
            {
                if (json_writer)
                    asterix.decodePCAPFile(filename, write_callback, flat);
                else  // printing done via flag
#if USE_OPENSSL
                    if (check_artas_md5_hash.size())
                        asterix.decodePCAPFile(filename, check_callback, flat);
                    else
                        asterix.decodePCAPFile(filename, empty_callback, flat);
#else
                    asterix.decodePCAPFile(filename, empty_callback, flat);
#endif
            }
            else if (framing == "netto" || framing == "")
            {
                if (json_writer)
                    asterix.decodeFile(filename, write_callback, flat);
                else  // printing done via flag
#if USE_OPENSSL
                    if (check_artas_md5_hash.size())
                        asterix.decodeFile(filename, check_callback, flat);
                    else
                        asterix.decodeFile(filename, empty_callback, flat);

#else
                    asterix.decodeFile(filename, empty_callback, flat);
#endif
            }
            else
            {
                if (json_writer)
                    asterix.decodeFile(filename, framing, write_callback, flat);
                else  // printing done via flag
                {
#if USE_OPENSSL
                    if (check_artas_md5_hash.size())
                        asterix.decodeFile(filename, framing, check_callback, flat);
                    else
                        asterix.decodeFile(filename, framing, empty_callback, flat);

#else
                    asterix.decodeFile(filename, framing, empty_callback, flat);
#endif
                }
            }
        }

#if USE_OPENSSL
        if (hash_checker)
            hash_checker->printCollisions();
#endif

        size_t num_frames = asterix.numFrames();
        size_t num_records = asterix.numRecords();

        boost::posix_time::time_duration diff =
                boost::posix_time::microsec_clock::local_time() - start_time;

        string time_str = to_string(diff.hours()) + "h " + to_string(diff.minutes()) + "m " +
                to_string(diff.seconds()) + "s " +
                to_string(diff.total_milliseconds() % 1000) + "ms";

        double seconds = diff.total_milliseconds() / 1000.0;

        if (log_performance)
            loginf << "jASTERIX client: decoded " << num_frames << " frames, " << num_records
                   << " records in " << time_str << ": " << num_frames / seconds << " fr/s, "
                   << num_records / seconds << " rec/s" << logendl;
    }
    catch (exception& ex)
    {
        logerr << "jASTERIX client: caught exception: " << ex.what() << logendl;

        // traced_assert(false);

        return -1;
    }
    catch (...)
    {
        logerr << "jASTERIX client: caught exception" << logendl;

        // traced_assert(false);

        return -1;
    }

    //    if (json_writer)
    //    {
    //        delete json_writer;
    //        json_writer = nullptr;
    //    }

    //#if USE_OPENSSL
    //    if (hash_checker)
    //    {
    //        delete hash_checker;
    //        hash_checker = nullptr;
    //    }
    //#endif

    if (debug)
        loginf << "jASTERIX client: shutdown" << logendl;

    return 0;
}
