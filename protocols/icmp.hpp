#ifndef PROTOCOLS_ICMP_H
#define PROTOCOLS_ICMP_H

#include <cstdint>
#include <array>


#pragma pack(push,1)
struct IcmpHeader {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t Extended_header;
};


#pragma pack(pop)



enum class icmp_types : uint8_t {
    Echo_Reply = 0,
    Dest_unreachable = 3,
    Redirect_Message  = 5,
    Echo_Request = 8,
    Time_Exceedced = 11,
    Parameter_Problem = 12,
};




#endif