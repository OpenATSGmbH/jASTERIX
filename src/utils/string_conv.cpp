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

#include "string_conv.h"
#include "traced_assert.h"

#include <cassert>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

int char2int(char input)
{
    if (input >= '0' && input <= '9')
        return input - '0';
    if (input >= 'A' && input <= 'F')
        return input - 'A' + 10;
    if (input >= 'a' && input <= 'f')
        return input - 'a' + 10;
    throw std::invalid_argument("Invalid input string");
}

// This function assumes src to be a zero terminated sanitized string with
// an even number of [0-9a-f] characters, and target to be sufficiently large
// Returns size
size_t hex2bin(const char* src, char* target)
{
    size_t src_len = strlen(src);
    traced_assert(src_len % 2 == 0);

    while (*src && src[1])
    {
        *(target++) = static_cast<char>(char2int(*src) * 16 + char2int(src[1]));
        src += 2;
    }

    return src_len / 2;
}

char getIcaoChar(unsigned char c)
{
    char ch;

    ch = '?';
    if (1 <= c && c <= 26)
    {
        ch = static_cast<char>('A' + (c - 1));
    }
    else if (c == 32)
    {
        ch = ' ';
    }
    else if (48 <= c && c <= 57)
    {
        ch = static_cast<char>('0' + (c - 48));
    }

    return ch;
}

constexpr char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                           '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

std::string binary2hex(const unsigned char* data, unsigned int len)
{
    std::string s(len * 2, ' ');
    for (unsigned int i = 0; i < len; ++i)
    {
        s[2 * i] = hexmap[(data[i] & 0xF0) >> 4];
        s[2 * i + 1] = hexmap[data[i] & 0x0F];
    }
    return s;
}

// std::string binary2hex(const unsigned char* src, unsigned int length)
//{
//    std::stringstream ss;
//    for(unsigned int i=0; i < length; ++i)
//        ss << std::setfill('0') << std::setw(2) << std::hex << (int)src[i];
//    return ss.str();
//}

std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems)
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

std::string toString(const nlohmann::json& j)
{
    if (j.type() == nlohmann::json::value_t::string)
    {
        return j.get<std::string>();
    }

    return j.dump();
}

bool isASCII(const std::string& s)
{
    return !std::any_of(s.begin(), s.end(),
                        [](char c) { return static_cast<unsigned char>(c) > 127; });
}

// std::vector<std::string> split(const std::string &s, char delim)
//{
//    std::vector<std::string> elems;
//    return split(s, delim, elems);
//}
