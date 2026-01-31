#ifndef PROTOCOLS_IPV4_H
#define PROTOCOLS_IPV4_H

#include "cstdint"
#include <array>

#pragma pack(push,1)
struct Ipv4Header {

    uint8_t version_ihl; //high 4bits version - low 4bits ip header length
    uint8_t dscp_ecn;  //type of service, low delay, high thoughtput reliability 8bits
    uint16_t total_length; // length of PROTOCOLS_IPV4_Hheader + data
    uint16_t identification; //unique packet id 
    uint16_t flags_fragment_offset; // 3 flags for 1 bit each, fragment offset: represents the number of data bytes ahead of the particular fragment
    uint8_t ttl; //time to live
    uint8_t protocol; //number of protocols to which data is to be passed
    uint16_t header_checksum; //for checking errors
    std::array<uint8_t, 4> src_ip;
    std::array<uint8_t, 4> dst_ip;
};
#pragma pack(pop)


static_assert(sizeof(Ipv4Header) == 20, "IPv4 Header must be size of 20 bytes");


constexpr uint8_t IPV4_VERSION = 4;
constexpr uint8_t IPV4_IHL_NO_OPTIONS = 5;
constexpr uint8_t IPPROTO_ICMP = 1;
constexpr uint8_t IPPROTO_TCP = 6;
constexpr uint8_t IPPROTO_UDP = 17;



#endif


