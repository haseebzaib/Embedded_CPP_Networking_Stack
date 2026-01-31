#ifndef NET_STACK_NETWORK_STACK_H
#define NET_STACK_NETWORK_STACK_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include "hal/hal_network.hpp"
#include "protocols/ethernet.hpp"
#include "protocols/arp.hpp"
#include "arp_cache.hpp"
#include "icmpStack.hpp"


namespace net {

	class NetworkStack {
	public:
		explicit NetworkStack(const NetworkConfig* config);

		/*Main processing loop*/
		void poll();

		/*Sends ARP request*/
		void send_arp_request_for_gateway();

		// Sends an ARP reply. Called by the ArpCache.
		void send_arp_reply(const std::array<uint8_t, IPV4_ADDRESS_LENGTH>& target_ip,
			const std::array<uint8_t, MAC_ADDRESS_LENGTH>& target_mac);

		// Getter for our configuration.
		const NetworkConfig* get_config() const { return m_config; }

		bool is_gateway_mac_known() ;

		ArpCache& get_arp_cache() ;

	private:
			enum class E_EtherType : uint16_t {
			Arp ,
			IpV4,
			IpV6,
			Unknown,
		};
	
		std::array<std::byte, 1514> m_packet_buffer;
		const NetworkConfig* m_config;
		uint32_t m_last_periodic_ms = 0;
		ArpCache m_arp_cache;
		ICMP_stack m_icmp;


	    E_EtherType decode_ether_type(uint16_t ethertype);
		void handle_arp_frame(std::span<const std::byte> frame);
		void handle_ipv4_frame(std::span<const std::byte> frame);
		void handle_ipv6_frame(std::span<const std::byte> frame);
		void handle_unknown_frame(std::span<const std::byte> frame);
		void process_incoming_frame(std::span<const std::byte> frame);


	};

}

#endif