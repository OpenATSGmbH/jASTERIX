#include "traced_assert.h"
#include "logger.h"

#include <cstdlib>
#include <sstream>
#include <iostream>

namespace compass_assert
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

        // msghandler::Message msg;
        // msg.severity    = msghandler::Severity::Abort;
        // msg.content     = expr_is_message ? std::string(expr) : "Assertion '" + std::string(expr) + "' failed";
        // msg.file        = std::string(file);
        // msg.line        = (int)line;
        // msg.stack_trace = ss.str();
        // msg.user_level  = false;

        //log assert msg
        // Check if log4cpp root category exists

        bool aborting = true;
        bool show_st = aborting && !stack_trace_str.empty();

        logerr << "Encountered critical error" << (aborting ? ", going into shutdown" : "")
                  << logendl
                  << logendl
                  << "Error:       " << (msg_content.empty() ? "Unknown error" : msg_content)
                  << logendl
                  //<< "Error Code:  " << msg.err_code << logendl
                  //<< "Component:   " << (msg.component.empty() ? "Unknown" : msg.component) << logendl
                  << "File:        " << std::string(file) << logendl
                  << "Line:        " << (int)line << logendl
                  << (show_st ? logendl : "") << (show_st ? stack_trace_str : "")
                  << (show_st ? logendl : "") << (aborting ? "Aborting..." : "");

        //then abort
        std::abort();
    }
}