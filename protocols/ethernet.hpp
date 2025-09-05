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
// We need to handle byte order (endianness) for these.
// Network byte order is big-endian. Most CPUs (x86, ARM) are little-endian.
// We will create a helper for this later.
constexpr uint16_t ETHERTYPE_IPV4 = 0x0800;
constexpr uint16_t ETHERTYPE_ARP = 0x0806;
constexpr uint16_t ETHERTYPE_IPV6 = 0x86DD;

#endif // PROTOCOLS_ETHERNET_H
