/*
 * This file is part of ATSDB.
 *
 * ATSDB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ATSDB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with ATSDB.  If not, see <http://www.gnu.org/licenses/>.
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
#include <thread>
#include <memory>

#include <tbb/tbb.h>

#if USE_LOG4CPP
#include "log4cpp/Layout.hh"
#include "log4cpp/OstreamAppender.hh"
#include "log4cpp/SimpleLayout.hh"
#endif

using namespace std;

extern std::unique_ptr<jASTERIX::JSONWriter> json_writer;

std::unique_ptr<jASTERIX::JSONWriter> json_writer;

void write_callback(std::unique_ptr<nlohmann::json> data_chunk, size_t num_frames,
                    size_t num_records, size_t num_errors)
{
    //    loginf << "jASTERIX: write_callback " << num_frames << " frames, " << num_records << "
    //    records, "
    //           << num_errors << " errors";

    traced_assert(json_writer);
    json_writer->write(std::move(data_chunk));
}

void empty_callback(std::unique_ptr<nlohmann::json> data_chunk, size_t num_frames,
                    size_t num_records, size_t num_errors)
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
    bool debug{false};
    bool debug_include_framing{false};
    bool print{false};
    bool print_cat_info{false};
    std::string write_type;
    std::string write_filename;
    bool log_performance{false};

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
                "log_perf", po::bool_switch(&log_performance), "enable performance log after processing")(
                "analyze", po::bool_switch(&analyze), "analyze data sources and contents")(
                "analyze_csv", po::bool_switch(&analyze_csv), "analyze data sources and contents, print as CSV")(
                "analyze_record_limit", po::value<unsigned int>(&analyze_record_limit),
                "number of records to analyze. 0 (default) disables limit.")
        #if USE_OPENSSL
            ("add_artas_md5", po::bool_switch(&jASTERIX::add_artas_md5_hash), "add ARTAS MD5 hashes")(
                "check_artas_md5", po::value<std::string>(&check_artas_md5_hash),
                "add and check ARTAS MD5 hashes (with record data), stating which categories to check, "
                "e.g. 1,20,21,48")
        #endif
            ("add_record_data", po::bool_switch(&jASTERIX::add_record_data),
             "add original record data in hex")("print", po::bool_switch(&print),
                                                "print JSON output")(
                "print_indent", po::value<int>(&jASTERIX::print_dump_indent),
                "intendation of json print, use -1 to disable.")(
                "write_type", po::value<std::string>(&write_type),
                "optional write type, e.g. text,zip. needs write_filename.")(
                "write_filename", po::value<std::string>(&write_filename),
                "optional write filename, e.g. test.zip.");

    try
    {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

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

            if (framing == "netto" || framing == "")
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
            if (framing == "netto" || framing == "")
            {
                if (json_writer)
                    asterix.decodeFile(filename, write_callback);
                else  // printing done via flag
#if USE_OPENSSL
                    if (check_artas_md5_hash.size())
                        asterix.decodeFile(filename, check_callback);
                    else
                        asterix.decodeFile(filename, empty_callback);

#else
                    asterix.decodeFile(filename, empty_callback);
#endif
            }
            else
            {
                if (json_writer)
                    asterix.decodeFile(filename, framing, write_callback);
                else  // printing done via flag
                {
#if USE_OPENSSL
                    if (check_artas_md5_hash.size())
                        asterix.decodeFile(filename, framing, check_callback);
                    else
                        asterix.decodeFile(filename, framing, empty_callback);

#else
                    asterix.decodeFile(filename, framing, empty_callback);
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
