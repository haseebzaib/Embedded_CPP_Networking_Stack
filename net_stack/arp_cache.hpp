#ifndef NET_STACK_ARP_CACHE_H
#define NET_STACK_ARP_CACHE_H


#include "array"
#include "cstdint"
#include "optional"

#include "protocols\arp.hpp"


//forward declaration to avoid circular dependencies
namespace net {
	class NetworkStack;
}

namespace net {

	enum class ArpEntryState {
		EMPTY,
		PENDING,
		RESOLVED
	};

	struct ArpEntry {
		ArpEntryState state = ArpEntryState::EMPTY;
		std::array<uint8_t, IPV4_ADDRESS_LENGTH> ipv4_address;
		std::array<uint8_t, MAC_ADDRESS_LENGTH> mac_address;
		uint32_t timestamp_ms = 0;
	};

    class ArpCache {
    public:
        // Tries to find the MAC address for a given IP.
        // Returns a std::optional containing the MAC address if found and resolved.
        std::optional<std::array<uint8_t, MAC_ADDRESS_LENGTH>> lookup(const std::array<uint8_t, IPV4_ADDRESS_LENGTH>& ip_address);

        // Called when we receive an ARP packet. It will update the cache and,
        // if it's a request for us, it will trigger the NetworkStack to send a reply.
        void process_arp_packet(NetworkStack& stack, const ArpPacket& packet);

        // Periodically called to clear out old entries.
        void age_entries(uint32_t current_time_ms);

        // Find an empty slot or the oldest entry to create a pending entry.
        void add_or_update_entry(const std::array<uint8_t, IPV4_ADDRESS_LENGTH>& ip_address,
            const std::array<uint8_t, MAC_ADDRESS_LENGTH>& mac_address,
            ArpEntryState new_state);

    private:
        // A fixed-size array for our cache. No dynamic allocation.
        static constexpr size_t CACHE_SIZE = 16;
        std::array<ArpEntry, CACHE_SIZE> m_entries;
    };





}



#endif