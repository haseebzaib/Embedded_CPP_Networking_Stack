#include "icmpStack.hpp"
#include "network_stack.hpp"
#include "hal/hal_logging.hpp"
#include "hal/hal_timer.hpp"
#include "byte_order.hpp"
#include "cstring"
#include <span>

namespace net
{

  uint16_t ICMP_stack::internet_checksum(std::span<const std::byte> bytes)
  {
    uint32_t sum = 0;
    const auto *p = reinterpret_cast<const uint8_t *>(bytes.data());
    size_t n = bytes.size();

    while (n >= 2)
    {
      uint16_t word = (uint16_t(p[0]) << 8) | uint16_t(p[1]); // big-endian
      sum += word;
      p += 2;
      n -= 2;
    }
    if (n == 1)
    {
      sum += uint16_t(p[0]) << 8; // pad low byte with 0
    }

    while (sum >> 16)
    {
      sum = (sum & 0xFFFFu) + (sum >> 16);
    }
    return uint16_t(~sum);
  }

  void ICMP_stack::process_echo_request(NetworkStack &stack, const EthernetHeader &rx_eth, const Ipv4Header &rx_ip, std::span<const std::byte> frame)
  {

    // Need at least: ICMP header (4) + identifier+sequence (4) = 8 bytes total
    if (frame.size() < (sizeof(IcmpHeader) + 4))
    {
        return;
    }

    // Parse identifier/sequence (network order on wire)
    std::span<const std::byte> echo_body = frame.subspan(sizeof(IcmpHeader));

    std::uint16_t id_be = 0;
    std::uint16_t seq_be = 0;

    std::memcpy(&id_be, echo_body.data(), sizeof(std::uint16_t));
    std::memcpy(&seq_be, echo_body.data() + 2, sizeof(std::uint16_t));

    std::uint16_t identifier = net_ntohs16(id_be);
    std::uint16_t sequence = net_ntohs16(seq_be);

    NET_LOG_DEBUG(ICMP, "EchoRequest id=%u seq=%u (len=%zu)", identifier, sequence, frame.size());

    // ---------------- Build Echo Reply ----------------
    const net::NetworkConfig *cfg = stack.get_config();
    if (cfg == nullptr)
    {
        return;
    }

    const size_t icmp_len = frame.size();
    const size_t ip_header_len = sizeof(Ipv4Header); // replying with no options (IHL=5)
    const size_t ip_total_len = ip_header_len + icmp_len;
    const size_t eth_total_len = sizeof(EthernetHeader) + ip_total_len;

    if (eth_total_len > 1514)
    {
        NET_LOG_DEBUG(ICMP, "Reply too large to send (%zu bytes)", eth_total_len);
        return;
    }

    std::array<std::byte, 1514> tx{};
    EthernetHeader *tx_eth = reinterpret_cast<EthernetHeader *>(tx.data());
    Ipv4Header *tx_ip = reinterpret_cast<Ipv4Header *>(tx.data() + sizeof(EthernetHeader));
    std::byte *tx_icmp_bytes = tx.data() + sizeof(EthernetHeader) + sizeof(Ipv4Header);

    // Ethernet: send back to requester MAC
    std::memcpy(tx_eth->destination_mac, rx_eth.source_mac, 6);
    std::memcpy(tx_eth->source_mac, cfg->mac_address.data(), 6);
    tx_eth->ethertype = net_htons16(ETHERTYPE_IPV4);

    // IPv4: build a fresh header (IHL=5, no options)
    tx_ip->version_ihl = static_cast<std::uint8_t>((IPV4_VERSION << 4) | IPV4_IHL_NO_OPTIONS);
    tx_ip->dscp_ecn = 0;
    tx_ip->total_length = net_htons16(static_cast<std::uint16_t>(ip_total_len));
    tx_ip->identification = 0; // ok for now
    tx_ip->flags_fragment_offset = net_htons16(0);
    tx_ip->ttl = 64;
    tx_ip->protocol = IPPROTO_ICMP;
    tx_ip->header_checksum = 0;
    tx_ip->src_ip = cfg->ipv4_address; // our IP
    tx_ip->dst_ip = rx_ip.src_ip;      // reply to sender IP

    // IPv4 header checksum (over 20 bytes)
    {
        const std::byte *ip_bytes_ptr = reinterpret_cast<const std::byte *>(tx_ip);
        std::span<const std::byte> ip_bytes(ip_bytes_ptr, sizeof(Ipv4Header));
        std::uint16_t ip_ck = internet_checksum(ip_bytes);
        tx_ip->header_checksum = net_htons16(ip_ck);
    }

    // ICMP: copy request, change type to EchoReply, recompute checksum
    std::memcpy(tx_icmp_bytes, frame.data(), icmp_len);

    IcmpHeader *tx_icmp = reinterpret_cast<IcmpHeader *>(tx_icmp_bytes);
    tx_icmp->type = static_cast<std::uint8_t>(icmp_types::EchoReply);
    tx_icmp->code = 0;
    tx_icmp->checksum = 0;

    {
        const std::byte *icmp_bytes_ptr = reinterpret_cast<const std::byte *>(tx_icmp);
        std::span<const std::byte> icmp_bytes(icmp_bytes_ptr, icmp_len);
        std::uint16_t icmp_ck = internet_checksum(icmp_bytes);
        tx_icmp->checksum = net_htons16(icmp_ck);
    }

    // Send
    (void)hal_net_send(tx.data(), eth_total_len);
    NET_LOG_DEBUG(ICMP, "EchoReply sent");
  }

  void ICMP_stack::process_icmp_packet(NetworkStack &stack, const EthernetHeader &rx_eth, const Ipv4Header &rx_ip, std::span<const std::byte> frame)
  {
    if (frame.size() < sizeof(IcmpHeader))
    {
        return;
    }

    const IcmpHeader *icmp_header = reinterpret_cast<const IcmpHeader *>(frame.data());
    std::uint16_t checksum = net_ntohs16(icmp_header->checksum);

    NET_LOG_DEBUG(ICMP, "Type|code|Checksum  %u|%u|%u",
                  static_cast<unsigned>(icmp_header->type),
                  static_cast<unsigned>(icmp_header->code),
                  checksum);

    icmp_types type = static_cast<icmp_types>(icmp_header->type);

    switch (type)
    {
    case icmp_types::EchoRequest:
        if (icmp_header->code == 0)
        {
            NET_LOG_DEBUG(ICMP, "EchoRequest");
            process_echo_request(stack, rx_eth, rx_ip, frame);
        }
        return;

    case icmp_types::EchoReply:
        NET_LOG_DEBUG(ICMP, "EchoReply (rx)");
        return;

    default:
        NET_LOG_DEBUG(ICMP, "ICMP type %u not handled", static_cast<unsigned>(icmp_header->type));
        return;
    }
  }

}