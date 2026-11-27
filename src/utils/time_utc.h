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

#include <ctime>

// UTC conversions between time_t and struct tm. gmtime_r and timegm are POSIX, Windows has
// gmtime_s (other argument order) and _mkgmtime.
namespace TimeUTC
{
inline bool toTm(time_t secs, struct tm& tm_utc)
{
#ifdef _WIN32
    return gmtime_s(&tm_utc, &secs) == 0;
#else
    return gmtime_r(&secs, &tm_utc) != nullptr;
#endif
}

// normalizes out-of-range fields, as timegm does
inline time_t fromTm(struct tm& tm_utc)
{
#ifdef _WIN32
    return _mkgmtime(&tm_utc);
#else
    return timegm(&tm_utc);
#endif
}
}  // namespace TimeUTC
