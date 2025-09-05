#ifndef NET_STACK_NETWORK_STACK_H
#define NET_STACK_NETWORK_STACK_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include "hal\hal_network.hpp"
#include "protocols\ethernet.hpp"
#include "protocols\arp.hpp"
#include "arp_cache.hpp"



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
		void process_incoming_frame(std::span<const std::byte> frame);
	
		std::array<std::byte, 1514> m_packet_buffer;
		const NetworkConfig* m_config;
		uint32_t m_last_periodic_ms = 0;
		ArpCache m_arp_cache;
	};

}

#endif