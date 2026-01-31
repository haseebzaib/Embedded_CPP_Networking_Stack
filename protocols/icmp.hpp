#ifndef PROTOCOLS_ICMP_H
#define PROTOCOLS_ICMP_H

#include <cstdint>
#include <array>


#pragma pack(push,1)
struct IcmpHeader {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
};


#pragma pack(pop)

static_assert(sizeof(IcmpHeader) == 4, "icmp Header must be size of 4 bytes");

enum class icmp_types : uint8_t {
    EchoReply = 0,
    DestinationUnreachable = 3,
    Redirect  = 5,
    EchoRequest = 8,
    TimeExceeded = 11,
    ParameterProblem = 12,
    TimestampRequest = 13,
    TimestampReply = 14
};




#endif