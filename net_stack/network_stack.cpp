#include "network_stack.hpp"
#include "hal/hal_network.hpp"
#include "hal/hal_timer.hpp"
#include "hal/hal_logging.hpp"
#include "protocols/arp.hpp"
#include "protocols/ethernet.hpp"
#include "iostream"
#include "cstring"
#include <span>
namespace net {

    static uint16_t htons(uint16_t hostshort)
    {
        uint16_t test = 1;
        if (*(reinterpret_cast<uint8_t*>(&test)) == 1) { // Little-endian check
            return (hostshort >> 8) | (hostshort << 8);
        }
        else {
            return hostshort;
        }
    }

    static uint16_t ntohs(uint16_t netshort) {
        return htons(netshort);
    }

    NetworkStack::NetworkStack(const NetworkConfig* config)
        : m_config(config) {
        // The constructor simply stores the configuration.
    }



    void NetworkStack::poll() {
        // 1. --- RECEIVE ---
       // Create a view into our buffer.
        std::span<std::byte> buffer_view(m_packet_buffer);

        // --- THIS IS THE KEY CHANGE ---
        // We now loop until the driver has no more packets to give us.
        // This drains the receive queue completely on every poll cycle.
        while (true) {
            size_t bytes_received = hal_net_receive(buffer_view.data(), buffer_view.size());

            if (bytes_received > 0) {
                // If we got a packet, process it immediately.
                NET_LOG_DEBUG(NET, "poll() received a frame of size: %zu ", bytes_received);
                process_incoming_frame({ buffer_view.data(), bytes_received });
            }
            else {
                // If bytes_received is 0, the driver's buffer is empty.
                // We can stop trying to receive and break the loop.
                break;
            }
        }

        // 2. --- PERIODIC TASKS ---
        // Later, this is where we would check timers for DHCP, TCP, etc.
        uint32_t current_time_ms = hal_timer_get_ms();
        if (current_time_ms - m_last_periodic_ms > 2000) { // Every 1 second
            m_arp_cache.age_entries(current_time_ms);
            m_last_periodic_ms = current_time_ms;
        }
    }


    void NetworkStack::process_incoming_frame(std::span<const std::byte> frame)
    {
        if (frame.size() < sizeof(EthernetHeader))
        {
            return; //mallperformed
        }

        const EthernetHeader* eth_header = reinterpret_cast<const EthernetHeader*>(frame.data());
        uint16_t ethertype = ntohs(eth_header->ethertype);
        if (ethertype == ETHERTYPE_ARP)
        {
            NET_LOG_DEBUG(NET, "Frame has EtherType 0x%04X", ethertype);
          
            std::span<const std::byte>  arp_payload = frame.subspan(sizeof(EthernetHeader));
            if (arp_payload.size() >= sizeof(ArpPacket)) {
                const ArpPacket* arp_packet = reinterpret_cast<const ArpPacket*>(arp_payload.data());
                // Hand off all ARP processing to the cache.
                m_arp_cache.process_arp_packet(*this, *arp_packet);
            }
        }
    }



    // Implementation for the new public function to send an ARP request.
    void NetworkStack::send_arp_request_for_gateway() {
        constexpr size_t packet_size = sizeof(EthernetHeader) + sizeof(ArpPacket);
        std::array<std::byte, packet_size> buffer;

        EthernetHeader* eth_header = reinterpret_cast<EthernetHeader*>(buffer.data());
        ArpPacket* arp_packet = reinterpret_cast<ArpPacket*>(buffer.data() + sizeof(EthernetHeader));

        // --- Fill in the Ethernet Header ---
        memset(eth_header->destination_mac, 0xFF, 6);
        memcpy(eth_header->source_mac, m_config->mac_address.data(), 6);
        eth_header->ethertype = htons(ETHERTYPE_ARP);

        // --- Fill in the ARP Packet ---
        arp_packet->hardware_type = htons(ARP_HW_TYPE_ETHERNET);
        arp_packet->protocol_type = htons(ETHERTYPE_IPV4);
        arp_packet->hardware_addr_len = 6;
        arp_packet->protocol_addr_len = 4;
        arp_packet->opcode = htons(ARP_OPCODE_REQUEST);
        memcpy(arp_packet->sender_mac, m_config->mac_address.data(), 6);
        memcpy(arp_packet->sender_ip, m_config->ipv4_address.data(), 4);
        memset(arp_packet->target_mac, 0x00, 6);
        memcpy(arp_packet->target_ip, m_config->gateway_address.data(), 4);

 

        NET_LOG_DEBUG(NET, "Sending ARP Request for gateway...");
        hal_net_send(buffer.data(), buffer.size());
    }


    void NetworkStack::send_arp_reply(const std::array<uint8_t, IPV4_ADDRESS_LENGTH>& target_ip,
        const std::array<uint8_t, MAC_ADDRESS_LENGTH>& target_mac) {
        constexpr size_t packet_size = sizeof(EthernetHeader) + sizeof(ArpPacket);
        std::array<std::byte, packet_size> packet_buffer;

        EthernetHeader* eth_header = reinterpret_cast<EthernetHeader*>(packet_buffer.data());
        ArpPacket* arp_packet = reinterpret_cast<ArpPacket*>(packet_buffer.data() + sizeof(EthernetHeader));

        // Fill Ethernet Header
        std::memcpy(eth_header->destination_mac, target_mac.data(), MAC_ADDRESS_LENGTH); // Send directly to the requester
        std::memcpy(eth_header->source_mac, m_config->mac_address.data(), MAC_ADDRESS_LENGTH);
        eth_header->ethertype = htons(ETHERTYPE_ARP);

        // Fill ARP Packet
        arp_packet->hardware_type = htons(ARP_HW_TYPE_ETHERNET);
        arp_packet->protocol_type = htons(ETHERTYPE_IPV4);
        arp_packet->hardware_addr_len = MAC_ADDRESS_LENGTH;
        arp_packet->protocol_addr_len = IPV4_ADDRESS_LENGTH;
        arp_packet->opcode = htons(ARP_OPCODE_REPLY);
        std::memcpy(arp_packet->sender_mac, m_config->mac_address.data(), MAC_ADDRESS_LENGTH);
        std::memcpy(arp_packet->sender_ip, m_config->ipv4_address.data(), IPV4_ADDRESS_LENGTH);
        std::memcpy(arp_packet->target_mac, target_mac.data(), MAC_ADDRESS_LENGTH);
        std::memcpy(arp_packet->target_ip, target_ip.data(), IPV4_ADDRESS_LENGTH);

        NET_LOG_DEBUG(NET, "Sending ARP reply...");
        hal_net_send(packet_buffer.data(), packet_buffer.size());
    }


    bool NetworkStack::is_gateway_mac_known()  {
        // We ask our ARP cache if it has an entry for the gateway's IP.
        // The lookup function returns a std::optional. If it has a value,
        // the lookup was successful.
        auto mac_address_ = m_arp_cache.lookup(m_config->gateway_address);

        if (mac_address_.has_value())
        {
            std::array<uint8_t, MAC_ADDRESS_LENGTH> logging = mac_address_.value();
            NET_LOG_DEBUG(NET, "MAC Address: %x:%x:%x:%x:%x:%x", logging[0], logging[1], logging[2], logging[3], logging[4], logging[5]);
        }


        return mac_address_.has_value();
    }

     ArpCache& NetworkStack::get_arp_cache()  {
        return m_arp_cache;
    }

}