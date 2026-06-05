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

#include "pcap/pcapreader.h"

#include "logger.h"

#include <pcap.h>
#include <pcap/sll.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <cstdio>
#include <ctime>
#include <sstream>

using namespace std;

// Linux cooked v2 capture header (libpcap may predate its definition)
#ifndef DLT_LINUX_SLL2
#define DLT_LINUX_SLL2 276
struct sll2_header
{
    uint16_t sll2_protocol;
    uint16_t sll2_reserved_mbz;
    uint32_t sll2_if_index;
    uint16_t sll2_hatype;
    uint8_t  sll2_pkttype;
    uint8_t  sll2_halen;
    uint8_t  sll2_addr[8];
};
#endif

namespace jASTERIX
{
// trampoline passed to pcap_next_ex handling; routes to the reader instance
void pcapReaderPacketHandler(unsigned char* user_data, const struct pcap_pkthdr* pkthdr,
                             const unsigned char* packet)
{
    reinterpret_cast<PcapReader*>(user_data)->digestPacket(pkthdr, packet);
}

PcapReader::PcapReader() = default;

PcapReader::~PcapReader() { close(); }

void PcapReader::close()
{
    if (pcap_file_)
    {
        pcap_close(pcap_file_);
        pcap_file_       = nullptr;
        link_layer_type_ = -1;
    }
}

bool PcapReader::open(const std::string& filename)
{
    close();

    char errbuf[PCAP_ERRBUF_SIZE];

    pcap_file_ = pcap_open_offline(filename.c_str(), errbuf);

    if (pcap_file_ == nullptr)
    {
        logerr << "PcapReader: open: opening '" << filename << "' failed: " << errbuf << logendl;
        return false;
    }

    link_layer_type_ = pcap_datalink(pcap_file_);
    reached_eof_     = false;

    loginf << "PcapReader: opened '" << filename << "' with link layer type " << link_layer_type_
           << logendl;

    return true;
}

bool PcapReader::hasUnknownHeaders() const
{
    return !unknown_link_types_.empty() || !unknown_ether_types_.empty() ||
           !unknown_ip_protocols_.empty();
}

void PcapReader::digestPacket(const struct pcap_pkthdr* pkthdr, const unsigned char* packet)
{
    if (link_layer_type_ == DLT_EN10MB)
    {
        const struct ether_header* eh = (const struct ether_header*)packet;
        digestEther(ntohs(eh->ether_type), pkthdr, packet + sizeof(struct ether_header),
                    sizeof(struct ether_header));
    }
    else if (link_layer_type_ == DLT_LINUX_SLL)
    {
        const struct sll_header* sll = (const struct sll_header*)packet;
        digestEther(ntohs(sll->sll_protocol), pkthdr, packet + sizeof(struct sll_header),
                    sizeof(struct sll_header));
    }
    else if (link_layer_type_ == DLT_LINUX_SLL2)
    {
        const struct sll2_header* sll2 = (const struct sll2_header*)packet;
        digestEther(ntohs(sll2->sll2_protocol), pkthdr, packet + sizeof(struct sll2_header),
                    sizeof(struct sll2_header));
    }
    else if (link_layer_type_ == DLT_RAW)
    {
        // raw IP packets, no link layer header
        digestEther(ETHERTYPE_IP, pkthdr, packet, 0);
    }
    else
    {
        unknown_link_types_.insert(link_layer_type_);
    }
}

void PcapReader::digestEther(int ether_type, const struct pcap_pkthdr* pkthdr,
                             const unsigned char* packet, unsigned long data_offs)
{
    char         source_ip[INET_ADDRSTRLEN];
    char         dest_ip[INET_ADDRSTRLEN];
    std::string  source_ip_str, dest_ip_str;
    unsigned int source_port = 0, dest_port = 0;
    uint8_t      ip_protocol = 0;
    unsigned int ip_version  = 0;

    const unsigned char* data        = nullptr;
    size_t               data_length = 0;

    if (ether_type == ETHERTYPE_IP)
    {
        const struct ip* ip_header = (const struct ip*)packet;
        inet_ntop(AF_INET, &(ip_header->ip_src), source_ip, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &(ip_header->ip_dst), dest_ip, INET_ADDRSTRLEN);

        source_ip_str = std::string(source_ip);
        dest_ip_str   = std::string(dest_ip);

        ip_protocol = ip_header->ip_p;
        ip_version  = ip_header->ip_v;

        if (ip_header->ip_p == IPPROTO_TCP)
        {
            const struct tcphdr* tcp_header = (const struct tcphdr*)(packet + sizeof(struct ip));
            source_port = ntohs(tcp_header->source);
            dest_port   = ntohs(tcp_header->dest);

            data        = packet + sizeof(struct ip) + sizeof(struct tcphdr);
            data_length = pkthdr->len - (data_offs + sizeof(struct ip) + sizeof(struct tcphdr));
        }
        else if (ip_header->ip_p == IPPROTO_UDP)
        {
            const struct udphdr* udp_header = (const struct udphdr*)(packet + sizeof(struct ip));
            source_port = ntohs(udp_header->source);
            dest_port   = ntohs(udp_header->dest);

            data = packet + sizeof(struct ip) + sizeof(struct udphdr);
            // use length from the udp header to drop any trailing padding
            data_length = ntohs(udp_header->len) - sizeof(struct udphdr);
        }
        else
        {
            unknown_ip_protocols_.insert(ip_header->ip_p);
        }
    }
    else
    {
        unknown_ether_types_.insert(ether_type);
    }

    // unsupported packet => drop
    if (!data)
    {
        ++num_packets_dropped_;
        return;
    }

    Signature signature(ip_protocol, ip_version, source_ip_str, source_port, dest_ip_str,
                        dest_port);

    // capture (network) timestamp of this packet, seconds since epoch (UTC)
    double timestamp = (double)pkthdr->ts.tv_sec + (double)pkthdr->ts.tv_usec / 1.0e6;

    addPayload(signature, data, data_length, timestamp);
}

void PcapReader::addPayload(const Signature& signature, const unsigned char* data, size_t len,
                            double timestamp)
{
    ++num_packets_read_;

    if (mode_ == Mode::PerSignature)
    {
        StreamData& stream = data_per_signature_[signature];
        ++stream.packets;
        stream.bytes += len;

        // packets are delivered in capture order, so first/last follow naturally
        if (!stream.has_time)
        {
            stream.first_time = timestamp;
            stream.has_time   = true;
        }
        stream.last_time = timestamp;

        if (stream.data.size() < max_bytes_per_sig_)
        {
            size_t can_add = std::min(len, max_bytes_per_sig_ - stream.data.size());
            stream.data.insert(stream.data.end(), data, data + can_add);
        }
    }
    else  // Mode::Chunk
    {
        // record where this packet's payload starts, so decoded data blocks can be
        // mapped back to the packet capture time
        chunk_packet_offsets_.emplace_back(chunk_buffer_.size(), timestamp);

        chunk_buffer_.insert(chunk_buffer_.end(), data, data + len);

        if (chunk_buffer_.size() >= chunk_max_bytes_)
            chunk_full_ = true;
    }
}

std::map<PcapReader::Signature, PcapReader::StreamData> PcapReader::readPerSignature(
    size_t max_bytes_per_signature)
{
    data_per_signature_.clear();

    if (!isOpen())
    {
        logerr << "PcapReader: readPerSignature: no file opened" << logendl;
        return {};
    }

    mode_              = Mode::PerSignature;
    max_bytes_per_sig_ = max_bytes_per_signature;

    struct pcap_pkthdr* pkthdr = nullptr;
    const u_char*       packet = nullptr;

    while (true)
    {
        int rc = pcap_next_ex(pcap_file_, &pkthdr, &packet);

        if (rc == 1)
        {
            if (pkthdr->caplen < pkthdr->len)  // truncated capture, skip
                continue;

            digestPacket(pkthdr, packet);
        }
        else if (rc == 0)
        {
            continue;  // timeout (live only), ignore
        }
        else if (rc == PCAP_ERROR_BREAK)
        {
            break;  // end of file
        }
        else
        {
            const char* err = pcap_geterr(pcap_file_);
            if (err && std::string(err).find("truncated dump file") != std::string::npos)
            {
                loginf << "PcapReader: readPerSignature: truncated dump tail ignored" << logendl;
                break;
            }

            logerr << "PcapReader: readPerSignature: pcap_next_ex error: "
                   << (err ? err : "unknown") << logendl;
            break;
        }
    }

    return std::move(data_per_signature_);
}

bool PcapReader::readNextChunk(std::vector<char>& out, size_t max_bytes, bool& eof)
{
    eof = false;

    if (!isOpen())
    {
        logerr << "PcapReader: readNextChunk: no file opened" << logendl;
        return false;
    }

    if (reached_eof_)
    {
        eof = true;
        return true;
    }

    mode_            = Mode::Chunk;
    chunk_max_bytes_ = max_bytes;
    chunk_full_      = false;
    chunk_buffer_.clear();
    chunk_packet_offsets_.clear();

    struct pcap_pkthdr* pkthdr = nullptr;
    const u_char*       packet = nullptr;

    while (!chunk_full_)
    {
        int rc = pcap_next_ex(pcap_file_, &pkthdr, &packet);

        if (rc == 1)
        {
            if (pkthdr->caplen < pkthdr->len)  // truncated capture, skip
                continue;

            digestPacket(pkthdr, packet);
        }
        else if (rc == 0)
        {
            continue;  // timeout (live only), ignore
        }
        else if (rc == PCAP_ERROR_BREAK)
        {
            reached_eof_ = true;
            break;
        }
        else
        {
            const char* err = pcap_geterr(pcap_file_);
            if (err && std::string(err).find("truncated dump file") != std::string::npos)
            {
                loginf << "PcapReader: readNextChunk: truncated dump tail ignored" << logendl;
                reached_eof_ = true;
                break;
            }

            logerr << "PcapReader: readNextChunk: pcap_next_ex error: " << (err ? err : "unknown")
                   << logendl;
            return false;
        }
    }

    out = std::move(chunk_buffer_);
    chunk_buffer_.clear();

    // signal eof only once the trailing (possibly empty) chunk has been delivered
    if (reached_eof_ && out.empty())
        eof = true;

    return true;
}

std::string PcapReader::signatureToString(const Signature& signature)
{
    uint8_t      ip_protocol = std::get<0>(signature);
    unsigned int ip_version  = std::get<1>(signature);

    std::stringstream ss;

    if (ip_protocol == IPPROTO_TCP)
        ss << "TCP";
    else if (ip_protocol == IPPROTO_UDP)
        ss << "UDP";
    else
        ss << "IPproto" << (unsigned int)ip_protocol;

    ss << " IPv" << ip_version << " ";
    ss << std::get<2>(signature) << ":" << std::get<3>(signature);
    ss << " => ";
    ss << std::get<4>(signature) << ":" << std::get<5>(signature);

    return ss.str();
}

std::string PcapReader::timeToString(double epoch_seconds)
{
    time_t secs = (time_t)epoch_seconds;
    int    ms   = (int)((epoch_seconds - (double)secs) * 1000.0 + 0.5);

    if (ms >= 1000)  // rounding carry
    {
        ms -= 1000;
        secs += 1;
    }

    struct tm tm_utc;
    gmtime_r(&secs, &tm_utc);

    char date_buf[32];
    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", &tm_utc);

    char out_buf[48];
    snprintf(out_buf, sizeof(out_buf), "%s.%03d UTC", date_buf, ms);

    return std::string(out_buf);
}
}  // namespace jASTERIX
