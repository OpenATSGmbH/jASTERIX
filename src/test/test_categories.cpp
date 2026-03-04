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
#include "logger.h"
#include "test_jasterix.h"

#if USE_LOG4CPP
#include "log4cpp/Layout.hh"
#include "log4cpp/OstreamAppender.hh"
#include "log4cpp/SimpleLayout.hh"
#endif

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

using namespace std;

std::string definition_path;
std::string data_path;

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
               Opt(data_path, "data_path")  // bind variable to a new option, with a hint string
                   ["--data_path"]          // the option names it will respond to
               ("path for data files");     // description string for the help output

    // Now pass the new composite back to Catch so it uses that
    session.cli(cli);

    // Let Catch (using Clara) parse the command line
    int returnCode = session.applyCommandLine(argc, argv);
    if (returnCode != 0)  // Indicates a command line error
        return returnCode;

    if (definition_path.size())
    {
        if (definition_path.back() != '/')
            definition_path += '/';
        loginf << "definition_path: '" << definition_path << "'" << logendl;
    }
    else
    {
        loginf << "definition_path variable missing" << logendl;
        return -1;
    }

    if (data_path.size())
    {
        if (data_path.back() != '/')
            data_path += '/';
        loginf << "data_path: '" << data_path << "'" << logendl;
    }
    else
    {
        loginf << "data_path variable missing" << logendl;
        return -1;
    }

    return session.run();
}
