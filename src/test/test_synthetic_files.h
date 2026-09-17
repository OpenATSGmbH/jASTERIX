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

// Builders for small synthetic recordings used by the framing, PCAP and limit tests. The
// files are built in memory around one CAT247 data block and written to a temp file.

#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <string>
#include <vector>

namespace synthetic
{
// CAT247 edition 1.2 data block, 20 bytes, one record (see test_cat247_1.2.cpp)
inline const std::vector<unsigned char> cat247_data_block = {
    0xf7, 0x00, 0x14, 0xf0, 0x00, 0x01, 0x01, 0x1c, 0x2f, 0xbb,
    0x03, 0x15, 0x02, 0x01, 0x17, 0x01, 0x02, 0xf7, 0x01, 0x02};

// one IOSS frame: length (2), unknown (1), board number (1), recording day (1),
// recording time (3, lsb 0.01 s), content, padding (4)
inline void appendIOSSFrame(std::vector<unsigned char>& out, unsigned int recording_day,
                            double recording_time_s, const std::vector<unsigned char>& content)
{
    unsigned int length = 8 + content.size() + 4;
    unsigned int time_raw = static_cast<unsigned int>(recording_time_s * 100.0 + 0.5);

    out.push_back((length >> 8) & 0xff);
    out.push_back(length & 0xff);
    out.push_back(0x00);  // unknown
    out.push_back(0x0f);  // board number
    out.push_back(recording_day & 0xff);
    out.push_back((time_raw >> 16) & 0xff);
    out.push_back((time_raw >> 8) & 0xff);
    out.push_back(time_raw & 0xff);
    out.insert(out.end(), content.begin(), content.end());
    out.insert(out.end(), {0xa5, 0xa5, 0xa5, 0xa5});  // padding
}

// RFF 128 byte file header: 20 byte start text, 20 byte stop text, 88 unknown bytes
inline void appendRFFHeader(std::vector<unsigned char>& out, const std::string& start_text,
                            const std::string& stop_text)
{
    auto append_field = [&out](const std::string& text) {
        std::string field = text;
        field.resize(20, ' ');
        out.insert(out.end(), field.begin(), field.end());
    };

    append_field(start_text);
    append_field(stop_text);
    out.insert(out.end(), 88, 0x00);
}

// one RFF frame: relative time ms (4, little-endian), length (2, little-endian), content
inline void appendRFFFrame(std::vector<unsigned char>& out, unsigned int relative_time_ms,
                           const std::vector<unsigned char>& content)
{
    unsigned int length = content.size();

    out.push_back(relative_time_ms & 0xff);
    out.push_back((relative_time_ms >> 8) & 0xff);
    out.push_back((relative_time_ms >> 16) & 0xff);
    out.push_back((relative_time_ms >> 24) & 0xff);
    out.push_back(length & 0xff);
    out.push_back((length >> 8) & 0xff);
    out.insert(out.end(), content.begin(), content.end());
}

inline void appendHost32(std::vector<unsigned char>& out, uint32_t value)
{
    unsigned char bytes[4];
    std::memcpy(bytes, &value, 4);
    out.insert(out.end(), bytes, bytes + 4);
}

inline void appendHost16(std::vector<unsigned char>& out, uint16_t value)
{
    unsigned char bytes[2];
    std::memcpy(bytes, &value, 2);
    out.insert(out.end(), bytes, bytes + 2);
}

inline void appendBE16(std::vector<unsigned char>& out, uint16_t value)
{
    out.push_back((value >> 8) & 0xff);
    out.push_back(value & 0xff);
}

// pcap global header, host byte order (the magic tells the reader the order), DLT_RAW
inline void appendPcapGlobalHeader(std::vector<unsigned char>& out)
{
    appendHost32(out, 0xa1b2c3d4);  // magic, microsecond timestamps
    appendHost16(out, 2);           // version major
    appendHost16(out, 4);           // version minor
    appendHost32(out, 0);           // this zone
    appendHost32(out, 0);           // sigfigs
    appendHost32(out, 65535);       // snaplen
    appendHost32(out, 12);          // link type DLT_RAW (raw IPv4/IPv6)
}

// one captured packet: pcap record header, IPv4 header, UDP header, payload
inline void appendPcapUDPPacket(std::vector<unsigned char>& out, time_t seconds,
                                uint32_t microseconds, const std::vector<unsigned char>& payload,
                                uint16_t destination_port = 5678)
{
    const uint16_t udp_length = 8 + payload.size();
    const uint16_t ip_length = 20 + udp_length;

    appendHost32(out, static_cast<uint32_t>(seconds));
    appendHost32(out, microseconds);
    appendHost32(out, ip_length);  // included length
    appendHost32(out, ip_length);  // original length

    // IPv4 header, 20 bytes
    out.push_back(0x45);  // version 4, header length 5 words
    out.push_back(0x00);  // tos
    appendBE16(out, ip_length);
    appendBE16(out, 0);       // identification
    appendBE16(out, 0x4000);  // flags: don't fragment
    out.push_back(64);        // ttl
    out.push_back(17);        // protocol UDP
    appendBE16(out, 0);       // checksum, not checked
    out.insert(out.end(), {10, 0, 0, 1});  // source 10.0.0.1
    out.insert(out.end(), {10, 0, 0, 2});  // destination 10.0.0.2

    // UDP header, 8 bytes
    appendBE16(out, 1234);
    appendBE16(out, destination_port);
    appendBE16(out, udp_length);
    appendBE16(out, 0);  // checksum

    out.insert(out.end(), payload.begin(), payload.end());
}

inline time_t utcSeconds(int year, int month, int day, int hours, int minutes, int seconds)
{
    struct tm tm_utc {};
    tm_utc.tm_year = year - 1900;
    tm_utc.tm_mon = month - 1;
    tm_utc.tm_mday = day;
    tm_utc.tm_hour = hours;
    tm_utc.tm_min = minutes;
    tm_utc.tm_sec = seconds;

    return timegm(&tm_utc);
}

// writes the bytes to a temp file named by tag and process id, returns the path
inline std::string writeTempFile(const std::string& tag, const std::vector<unsigned char>& bytes,
                                 const std::string& extension = ".bin")
{
    std::string path = std::string(P_tmpdir) + "/jasterix_test_" + tag + "_"
                       + std::to_string(getpid()) + extension;

    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    out.close();

    return path;
}
}  // namespace synthetic
