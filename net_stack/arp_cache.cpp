#include "arp_cache.hpp"
#include "network_stack.hpp"
#include "hal\hal_logging.hpp"
#include "hal\hal_timer.hpp"
#include "cstring"


namespace net {

    static constexpr uint32_t ARP_ENTRY_TIMEOUT_MS = 5 * 60 * 1000;

    std::optional<std::array<uint8_t, MAC_ADDRESS_LENGTH>>
        ArpCache::lookup(const std::array<uint8_t, IPV4_ADDRESS_LENGTH>& ip_to_find)  {
        hal_log(LogLevel::DEBUG, "--- ARP Cache Lookup ---");
        hal_log(LogLevel::DEBUG, "Searching for IP: %d.%d.%d.%d",
            ip_to_find[0], ip_to_find[1], ip_to_find[2], ip_to_find[3]);

        for (const auto& entry : m_entries) {
            if (entry.state == ArpEntryState::RESOLVED) {
                // Compare the entry's IP with the IP we are looking for.
                if (std::memcmp(entry.ipv4_address.data(), ip_to_find.data(), IPV4_ADDRESS_LENGTH) == 0) {
                    hal_log(LogLevel::DEBUG, "  MATCH FOUND!");
                    return entry.mac_address; // Found it!
                }
            }
        }

        hal_log(LogLevel::DEBUG, "  No match found in cache.");
        return std::nullopt; // Did not find it.
    }

    // Called when we receive an ARP packet. It will update the cache and,
    // if it's a request for us, it will trigger the NetworkStack to send a reply.
    void ArpCache::process_arp_packet(NetworkStack& stack, const ArpPacket& packet)
    {
        /*View the data for easier handling and debugging*/
        /*TODO: find any other implementation to improve it and use less memory*/
        std::array<uint8_t, IPV4_ADDRESS_LENGTH> sender_ip;
        std::array<uint8_t, MAC_ADDRESS_LENGTH> sender_mac;
        std::memcpy(sender_ip.data(), packet.sender_ip, IPV4_ADDRESS_LENGTH);
        std::memcpy(sender_mac.data(), packet.sender_mac, MAC_ADDRESS_LENGTH);

        // Add or update the sender's information in the cache now.
        add_or_update_entry(sender_ip, sender_mac, ArpEntryState::RESOLVED);

        // Now, check if this packet is a request specifically for us.
        if (packet.opcode == ARP_OPCODE_REQUEST) {
            // Is the target IP in the packet the same as our IP?
            if (std::memcmp(packet.target_ip, stack.get_config()->ipv4_address.data(), IPV4_ADDRESS_LENGTH) == 0) {
                hal_log(LogLevel::INFO, "Received an ARP request for our IP. Sending reply...");
                // The stack is responsible for constructing and sending the actual packet.
                stack.send_arp_reply(sender_ip, sender_mac);
            }
        }
    }

    // Periodically called to clear out old entries.
    void ArpCache::age_entries(uint32_t current_time_ms)
    {
        for (auto& entry : m_entries) {
            // We only care about entries that are currently resolved.
            if (entry.state == ArpEntryState::RESOLVED) {
                if (current_time_ms - entry.timestamp_ms > ARP_ENTRY_TIMEOUT_MS) {
                    // This entry is too old. Invalidate it.
                    hal_log(LogLevel::DEBUG, "ARP entry expired. Clearing.");
                    entry.state = ArpEntryState::EMPTY;
                }
            }
        }
    }

    // Find an empty slot or the oldest entry to create a pending entry.
    void ArpCache::add_or_update_entry(const std::array<uint8_t, IPV4_ADDRESS_LENGTH>& ip_address,
        const std::array<uint8_t, MAC_ADDRESS_LENGTH>& mac_address,
        ArpEntryState new_state)
    {
        // First, try to find an existing entry for this IP to update it.
        for (auto& entry : m_entries) {
            if (entry.state != ArpEntryState::EMPTY &&
                std::memcmp(entry.ipv4_address.data(), ip_address.data(), IPV4_ADDRESS_LENGTH) == 0)
            {
                // Found an existing entry. Update it.
                entry.mac_address = mac_address;
                entry.state = new_state;
                entry.timestamp_ms = hal_timer_get_ms();
                hal_log(LogLevel::DEBUG, "Updated ARP cache entry.");
                return;
            }
        }

        // If we got here, there's no existing entry. Find an empty slot.
        for (auto& entry : m_entries) {
            if (entry.state == ArpEntryState::EMPTY) {
                entry.ipv4_address = ip_address;
                entry.mac_address = mac_address;
                entry.state = new_state;
                entry.timestamp_ms = hal_timer_get_ms();
                hal_log(LogLevel::DEBUG, "Added new ARP cache entry.");
                return;
            }
        }

        // If we are still here, the cache is full. We would implement a
        // strategy to replace the oldest entry here, but for now, we'll just log it.
        hal_log(LogLevel::WARN, "ARP Cache is full! Could not add new entry.");
    }




}