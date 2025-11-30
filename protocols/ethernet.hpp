#ifndef PROTOCOLS_ETHERNET_H
#define PROTOCOLS_ETHERNET_H

#include <cstdint>
#include <array>
// Use pragma pack for cross-compiler portability (MSVC, GCC, Clang)
#pragma pack(push, 1)
struct EthernetHeader {
    uint8_t destination_mac[6];
    uint8_t source_mac[6];
    uint16_t ethertype;
};
#pragma pack(pop)

// Compile-time check to ensure the struct is packed correctly.
static_assert(sizeof(EthernetHeader) == 14, "EthernetHeader size must be 14 bytes");

// Common EtherType values.
constexpr uint16_t ETHERTYPE_IPV4 = 0x0800;  //host order; hton when writing to the wire
constexpr uint16_t ETHERTYPE_ARP = 0x0806;   //host order; hton when writing to the wire
constexpr uint16_t ETHERTYPE_IPV6 = 0x86DD;  //host order; hton when writing to the wire

#endif // PROTOCOLS_ETHERNET_H
