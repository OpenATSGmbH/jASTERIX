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

#include <sys/resource.h>

#include <fstream>
#include <limits>
#include <sstream>
#include <string>

#include "logger.h"

const double megabyte = 1024 * 1024;
const double gigabyte = 1024 * 1024 * 1024;

namespace Utils
{
namespace System
{
float getProcessRAMinGB()
{
    struct rusage info;
    getrusage(RUSAGE_SELF, &info);

    //    long int ru_maxrss
    //    The maximum resident set size used, in kilobytes. That is, the maximum number of kilobytes
    //    of physical memory that processes used simultaneously.

    return (info.ru_maxrss) / megabyte;
}

float getFreeRAMinGB()
{
    std::string token;
    std::ifstream file("/proc/meminfo");
    while (file >> token)
    {
        if (token == "MemAvailable:")
        {
            unsigned long mem;

            if (file >> mem)  // returns in kB
            {
                return mem / megabyte;
            }
            else
            {
                return 0;
            }
        }
        // ignore rest of the line
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    return 0;  // nothing found
}

}  // namespace System
}  // namespace Utils

