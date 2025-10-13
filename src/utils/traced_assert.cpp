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
 
#include "traced_assert.h"
#include "logger.h"

#include <cstdlib>
#include <sstream>
#include <iostream>

namespace jasterix_assert
{
    void assertion_failed(char const * expr, 
                                 char const * function,
                                 char const * file, 
                                 long line,
                                 const boost::stacktrace::stacktrace& stack_trace,
                                 bool expr_is_message)
    {
        std::string stack_trace_str;

        {
            std::stringstream ss;
            ss << stack_trace;

            stack_trace_str = ss.str();
        }

        //compile message

        std::string msg_content = expr_is_message ? std::string(expr) : "Assertion '" + std::string(expr) + "' failed";

        //log assert msg

        bool aborting = true;
        bool show_st = aborting && !stack_trace_str.empty();

        logerr << "Encountered critical error" << (aborting ? ", going into shutdown" : "")
                  << logendl
                  << logendl
                  << "Error:       " << (msg_content.empty() ? "Unknown error" : msg_content)
                  << logendl
                  << "File:        " << std::string(file) << logendl
                  << "Line:        " << (int)line << logendl
                  << (show_st ? logendl : "") << (show_st ? stack_trace_str : "")
                  << (show_st ? logendl : "") << (aborting ? "Aborting..." : "");

        //then abort
        std::abort();
    }
}