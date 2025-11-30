#ifndef PROTOCOLS_ARP_H
#define PROTOCOLS_ARP_H

#include <cstdint>

#pragma pack(push,1)
struct ArpPacket {
	uint16_t hardware_type;
	uint16_t protocol_type;
	uint8_t hardware_addr_len;
	uint8_t protocol_addr_len;
	uint16_t opcode;
	uint8_t sender_mac[6];
	uint8_t sender_ip[4];
	uint8_t target_mac[6];
	uint8_t target_ip[4];
};
#pragma pack(pop)


//Verifying size at compile time,An Arp packet of IPv4 is 28 bytes
static_assert(sizeof(ArpPacket) == 28, "ArpPacket size is incorrect!");

//ARP constants
constexpr uint16_t ARP_HW_TYPE_ETHERNET = 1; //correct formatted for big endian
constexpr uint16_t ARP_PROTO_TYPE_IPV4 = 0x0800; //host order; hton when writing to the wire
constexpr uint16_t ARP_OPCODE_REQUEST = 1; //host order; hton when writing to the wire
constexpr uint16_t ARP_OPCODE_REPLY = 2; //host order; hton when writing to the wire

constexpr size_t IPV4_ADDRESS_LENGTH = 4;
constexpr size_t MAC_ADDRESS_LENGTH = 6;


#endif // PROTOCOLS_ARP_H