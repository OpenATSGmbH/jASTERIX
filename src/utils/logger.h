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

#include <iostream>

#include "jasterix/global.h"

#if USE_LOG4CPP
#include "log4cpp/Category.hh"
#endif

namespace jASTERIX
{
#if USE_LOG4CPP
#define logerr log4cpp::Category::getRoot().errorStream()
//#define logwrn log4cpp::Category::getRoot().warnStream()
#define loginf log4cpp::Category::getRoot().infoStream()
//#define logdbg log4cpp::Category::getRoot().debugStream()
//#define logdbg if(0) log4cpp::Category::getRoot().debugStream() // for improved
#define logendl ""

#else

#define logerr std::cerr
#define loginf std::cout
#define logendl std::endl

#endif

}  // namespace jASTERIX


