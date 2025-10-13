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

#include "files.h"
#include "jasterix.h"
#include "logger.h"
#include "system.h"
#include "test_jasterix.h"

#if USE_LOG4CPP
#include "log4cpp/Layout.hh"
#include "log4cpp/OstreamAppender.hh"
#include "log4cpp/SimpleLayout.hh"
#endif

#define CATCH_CONFIG_RUNNER
#include <chrono>
#include <thread>

#include "catch.hpp"

using namespace std;

std::string definition_path;
std::string filename;
float max_used_ram_mb{0};
unsigned int sum_num_frames{0};
unsigned int sum_num_records{0};

std::chrono::time_point<std::chrono::steady_clock> start_time;
std::chrono::time_point<std::chrono::steady_clock> current_time;

void test_performance_callback(std::unique_ptr<nlohmann::json> json_data, size_t num_frames,
                               size_t num_records, size_t num_errors)
{
    // float free_ram = Utils::System::getFreeRAMinGB();

    float used_ram_mb = Utils::System::getProcessRAMinGB() * 1024.0;

    if (used_ram_mb > max_used_ram_mb)
        max_used_ram_mb = used_ram_mb;

    sum_num_frames += num_frames;
    sum_num_records += num_records;

    REQUIRE(num_errors == 0);

    auto current_time = chrono::steady_clock::now();
    double full_seconds =
        chrono::duration_cast<chrono::milliseconds>(current_time - start_time).count() / 1000.0;

    std::ostringstream oss;

    unsigned int hours = full_seconds / 3600;
    unsigned int minutes = (full_seconds - hours * 3600) / 60;
    double seconds = full_seconds - hours * 3600 - minutes * 60;

    oss << std::setfill('0') << std::setw(2) << hours << ":" << std::setw(2) << minutes << ":"
        << std::fixed << std::setw(6) << std::setprecision(3) << seconds;

    loginf << "performance test: decoded " << sum_num_frames << " frames, " << sum_num_records
           << " records in " << oss.str() << ": " << sum_num_frames / full_seconds << " fr/s, "
           << sum_num_records / full_seconds << " rec/s using " << std::fixed
           << std::setprecision(2) << used_ram_mb << " mb, max " << max_used_ram_mb << logendl;

    json_data = nullptr;
}

TEST_CASE("jASTERIX Performance", "[jASTERIX Performance]")
{
    loginf << "performance test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, false, false, false);
#if USE_OPENSSL
    jASTERIX::add_artas_md5_hash = true;
#endif

    REQUIRE(jASTERIX::Files::fileExists(filename));

    start_time = chrono::steady_clock::now();

    jasterix.decodeFile(filename, "ioss", test_performance_callback);

    loginf << "performance test: end" << logendl;

    // std::this_thread::sleep_for(std::chrono::seconds(5));
}

int main(int argc, char **argv)
{
    static_assert(sizeof(size_t) >= 8, "code requires size_t with at least 8 bytes");

    // setup logging
#if USE_LOG4CPP
    log4cpp::Appender *console_appender_ = new log4cpp::OstreamAppender("console", &std::cout);
    console_appender_->setLayout(new log4cpp::SimpleLayout());

    log4cpp::Category &root = log4cpp::Category::getRoot();
    root.setPriority(log4cpp::Priority::INFO);
    root.addAppender(console_appender_);
#endif

    Catch::Session session;

    // Build a new parser on top of Catch's
    using namespace Catch::clara;
    auto cli = session.cli()  // Get Catch's composite command line parser
               | Opt(definition_path,
                     "definition_path")     // bind variable to a new option, with a hint string
                     ["--definition_path"]  // the option names it will respond to
               ("path for definition files") |
               Opt(filename, "filename")     // bind variable to a new option, with a hint string
                   ["--filename"]            // the option names it will respond to
               ("path for file to decode");  // description string for the help output

    // Now pass the new composite back to Catch so it uses that
    session.cli(cli);

    // Let Catch (using Clara) parse the command line
    int returnCode = session.applyCommandLine(argc, argv);
    if (returnCode != 0)  // Indicates a command line error
        return returnCode;

    if (definition_path.size())
        loginf << "definition_path: '" << definition_path << "'" << logendl;
    else
    {
        loginf << "definition_path variable missing" << logendl;
        return -1;
    }

    if (filename.size())
        loginf << "filename: '" << filename << "'" << logendl;
    else
    {
        loginf << "filename variable missing" << logendl;
        return -1;
    }

    return session.run();
}
