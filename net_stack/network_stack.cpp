#include "network_stack.hpp"
#include "hal/hal_network.hpp"
#include "hal/hal_timer.hpp"
#include "hal/hal_logging.hpp"
#include "protocols/arp.hpp"
#include "protocols/ethernet.hpp"
#include "protocols/ipv4.hpp"
#include "byte_order.hpp"
#include "iostream"
#include "cstring"

#include <span>
namespace net
{

    NetworkStack::NetworkStack(const NetworkConfig *config)
        : m_config(config)
    {
        // The constructor simply stores the configuration.
    }

    void NetworkStack::poll()
    {
        // 1. --- RECEIVE ---
        // Create a view into our buffer.
        std::span<std::byte> buffer_view(m_packet_buffer);

        // --- THIS IS THE KEY CHANGE ---
        // We now loop until the driver has no more packets to give us.
        // This drains the receive queue completely on every poll cycle.
        while (true)
        {
            size_t bytes_received = hal_net_receive(buffer_view.data(), buffer_view.size());

            if (bytes_received > 0)
            {
                // If we got a packet, process it immediately.
                NET_LOG_DEBUG(NET, "poll() received a frame of size: %zu ", bytes_received);
                process_incoming_frame({buffer_view.data(), bytes_received});
            }
            else
            {
                // If bytes_received is 0, the driver's buffer is empty.
                // We can stop trying to receive and break the loop.
                break;
            }
        }

        // 2. --- PERIODIC TASKS ---
        // Later, this is where we would check timers for DHCP, TCP, etc.
        uint32_t current_time_ms = hal_timer_get_ms();
        if (current_time_ms - m_last_periodic_ms > 2000)
        { // Every 1 second
            m_arp_cache.age_entries(current_time_ms);
            m_last_periodic_ms = current_time_ms;
        }
    }

    // Implementation for the new public function to send an ARP request.
    void NetworkStack::send_arp_request_for_gateway()
    {
        constexpr size_t packet_size = sizeof(EthernetHeader) + sizeof(ArpPacket);
        std::array<std::byte, packet_size> buffer;

        EthernetHeader *eth_header = reinterpret_cast<EthernetHeader *>(buffer.data());
        ArpPacket *arp_packet = reinterpret_cast<ArpPacket *>(buffer.data() + sizeof(EthernetHeader));

        // --- Fill in the Ethernet Header ---
        memset(eth_header->destination_mac, 0xFF, 6);
        memcpy(eth_header->source_mac, m_config->mac_address.data(), 6);
        eth_header->ethertype = net_htons16(ETHERTYPE_ARP);

        // --- Fill in the ARP Packet ---
        arp_packet->hardware_type = net_htons16(ARP_HW_TYPE_ETHERNET);
        arp_packet->protocol_type = net_htons16(ETHERTYPE_IPV4);
        arp_packet->hardware_addr_len = 6;
        arp_packet->protocol_addr_len = 4;
        arp_packet->opcode = net_htons16(ARP_OPCODE_REQUEST);
        memcpy(arp_packet->sender_mac, m_config->mac_address.data(), 6);
        memcpy(arp_packet->sender_ip, m_config->ipv4_address.data(), 4);
        memset(arp_packet->target_mac, 0x00, 6);
        memcpy(arp_packet->target_ip, m_config->gateway_address.data(), 4);

        NET_LOG_DEBUG(NET, "Sending ARP Request for gateway...");
        hal_net_send(buffer.data(), buffer.size());
    }

    void NetworkStack::send_arp_reply(const std::array<uint8_t, IPV4_ADDRESS_LENGTH> &target_ip,
                                      const std::array<uint8_t, MAC_ADDRESS_LENGTH> &target_mac)
    {
        constexpr size_t packet_size = sizeof(EthernetHeader) + sizeof(ArpPacket);
        std::array<std::byte, packet_size> packet_buffer;

        EthernetHeader *eth_header = reinterpret_cast<EthernetHeader *>(packet_buffer.data());
        ArpPacket *arp_packet = reinterpret_cast<ArpPacket *>(packet_buffer.data() + sizeof(EthernetHeader));

        // Fill Ethernet Header
        std::memcpy(eth_header->destination_mac, target_mac.data(), MAC_ADDRESS_LENGTH); // Send directly to the requester
        std::memcpy(eth_header->source_mac, m_config->mac_address.data(), MAC_ADDRESS_LENGTH);
        eth_header->ethertype = net_htons16(ETHERTYPE_ARP);

        // Fill ARP Packet
        arp_packet->hardware_type = net_htons16(ARP_HW_TYPE_ETHERNET);
        arp_packet->protocol_type = net_htons16(ETHERTYPE_IPV4);
        arp_packet->hardware_addr_len = MAC_ADDRESS_LENGTH;
        arp_packet->protocol_addr_len = IPV4_ADDRESS_LENGTH;
        arp_packet->opcode = net_htons16(ARP_OPCODE_REPLY);
        std::memcpy(arp_packet->sender_mac, m_config->mac_address.data(), MAC_ADDRESS_LENGTH);
        std::memcpy(arp_packet->sender_ip, m_config->ipv4_address.data(), IPV4_ADDRESS_LENGTH);
        std::memcpy(arp_packet->target_mac, target_mac.data(), MAC_ADDRESS_LENGTH);
        std::memcpy(arp_packet->target_ip, target_ip.data(), IPV4_ADDRESS_LENGTH);

        NET_LOG_DEBUG(NET, "Sending ARP reply...");
        hal_net_send(packet_buffer.data(), packet_buffer.size());
    }

    bool NetworkStack::is_gateway_mac_known()
    {
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

    ArpCache &NetworkStack::get_arp_cache()
    {
        return m_arp_cache;
    }

    /* Private functions */

    NetworkStack::E_EtherType NetworkStack::decode_ether_type(uint16_t ethertype)
    {
        NET_LOG_DEBUG(NET, "Frame has EtherType 0x%04X", ethertype);
        E_EtherType EtherType;
        switch (ethertype)
        {
        case ETHERTYPE_IPV4:
        {

            EtherType = E_EtherType::IpV4;
            break;
        }

        case ETHERTYPE_ARP:
        {
            EtherType = E_EtherType::Arp;
            break;
        }

        case ETHERTYPE_IPV6:
        {
            EtherType = E_EtherType::IpV6;
            break;
        }

        default:
        {
            EtherType = E_EtherType::Unknown;
            break;
        }
        }

        return EtherType;
    }

    void NetworkStack::handle_arp_frame(std::span<const std::byte> frame)
    {
        std::span<const std::byte> arp_payload = frame.subspan(sizeof(EthernetHeader));
        if (arp_payload.size() >= sizeof(ArpPacket))
        {
            const ArpPacket *arp_packet = reinterpret_cast<const ArpPacket *>(arp_payload.data());
            // Hand off all ARP processing to the cache.
            m_arp_cache.process_arp_packet(*this, *arp_packet);
        }
    }
    void NetworkStack::handle_ipv4_frame(std::span<const std::byte> frame)
    {
        std::span<const std::byte> ipv4_payload = frame.subspan(sizeof(EthernetHeader));

        if (ipv4_payload.size() >= sizeof(Ipv4Header))
        {
            const Ipv4Header *ipv4_header = reinterpret_cast<const Ipv4Header *>(ipv4_payload.data());

  
            if(ipv4_header->dst_ip != m_config->ipv4_address)
            {
               return;
            }

            net::Split<std::uint8_t> s;

            s.u8 = ipv4_header->version_ihl;
            NET_LOG_DEBUG(NET, "Ipv4 HeaderLength: %d:%d | Ipv4 Version: %d:%d", s.le.low * 4, s.low() * 4, s.le.high, s.high());

            uint16_t total_length = net_ntohs16(ipv4_header->total_length);
            NET_LOG_DEBUG(NET, "Total length: %d", total_length);

            uint16_t ihl_bytes = s.low() * 4;

            if (total_length <= ipv4_payload.size() && s.high() == IPV4_VERSION && s.low() >= 5 && (s.low() * 4) <= ipv4_payload.size() && (s.low() * 4) <= total_length )
            {
                NET_LOG_DEBUG(NET, "Header is valid");

                /*Identification*/
                uint16_t identification = net_ntohs16(ipv4_header->identification);
                NET_LOG_DEBUG(NET, "Identification: %d", identification);

                /*Fragmentation detection */
                uint16_t flags_fragment_offset = net_ntohs16(ipv4_header->flags_fragment_offset);
                uint8_t Res_bit = static_cast<uint8_t>(extract_bits<uint16_t,15,1>(flags_fragment_offset)); 
                uint8_t DF_bit  = static_cast<uint8_t>(extract_bits<uint16_t,14,1>(flags_fragment_offset)); 
                uint8_t MF_bit  = static_cast<uint8_t>(extract_bits<uint16_t,13,1>(flags_fragment_offset)); 
                uint16_t fragment_offset_bits = extract_bits<uint16_t,0,13>(flags_fragment_offset); 
                uint16_t fragment_offset_bytes = fragment_offset_bits*8;

                NET_LOG_DEBUG(NET, "Res:%d|DF:%d|MF:%d|Fragment_Offset:%d|Fragment_Offset_Bytes:%d",Res_bit,DF_bit,MF_bit,fragment_offset_bits,fragment_offset_bytes);
 
                NET_LOG_DEBUG(NET, "Time to Live: %d|Protocol: %d|HeaderChecksum: %d", ipv4_header->ttl,ipv4_header->protocol,net_ntohs16(ipv4_header->header_checksum));

                NET_LOG_DEBUG(NET, "SrcIP:%d.%d.%d.%d | DstIP:%d.%d.%d.%d",ipv4_header->src_ip[0],ipv4_header->src_ip[1],ipv4_header->src_ip[2],ipv4_header->src_ip[3] 
                                                                        ,ipv4_header->dst_ip[0],ipv4_header->dst_ip[1],ipv4_header->dst_ip[2],ipv4_header->dst_ip[3]);

                if(Res_bit != 0)
                {
                   NET_LOG_DEBUG(NET, "Res bit is not 0 so dropping"); 
                   return;    
                }

                if(MF_bit==1 || fragment_offset_bits!=0)
                {
                    NET_LOG_DEBUG(NET, "Packet is fragmented so dropping for now (Fragmentation not supported)");
                    return;
                }

                std::span<const std::byte> ipv4_packet = ipv4_payload.first(total_length);

                std::span<const std::byte> L4_payload = ipv4_packet.subspan(ihl_bytes);

                if(ipv4_header->protocol == IPPROTO_ICMP)
                {
                  NET_LOG_DEBUG(NET, "ICMP detected");
                  m_icmp.process_icmp_packet(*this,L4_payload);
                  
                  return;
                }
        
            }
            else
            {
                NET_LOG_DEBUG(NET, "Dont trust the packet");
            }
        }
    }
    void NetworkStack::handle_ipv6_frame(std::span<const std::byte> frame)
    {
    }
    void NetworkStack::handle_unknown_frame(std::span<const std::byte> frame)
    {
    }

    void NetworkStack::process_incoming_frame(std::span<const std::byte> frame)
    {
        if (frame.size() < sizeof(EthernetHeader))
        {
            return; // mallperformed
        }

        const EthernetHeader *eth_header = reinterpret_cast<const EthernetHeader *>(frame.data());
        E_EtherType ethertype = decode_ether_type(net_ntohs16(eth_header->ethertype));

        switch (ethertype)
        {
        case E_EtherType::Arp:
            /* code */
            NET_LOG_DEBUG(NET, "ARP frame detected");
            handle_arp_frame(frame);
            break;

        case E_EtherType::IpV4:
            /* code */
            NET_LOG_DEBUG(NET, "IPV4 frame detected");
            handle_ipv4_frame(frame);
            break;

        case E_EtherType::IpV6:
            /* code */
            handle_ipv6_frame(frame);
            break;

        case E_EtherType::Unknown:
            /* code */
            handle_unknown_frame(frame);
            break;

        default:
            break;
        }
    }

}