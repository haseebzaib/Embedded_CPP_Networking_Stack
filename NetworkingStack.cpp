// NetworkingStack.cpp : Defines the entry point for the application.
//

#include "NetworkingStack.h"
#include "hal/hal_network.hpp"
#include "hal/hal_timer.hpp"
#include "hal/hal_logging.hpp"

#include "protocols/ethernet.hpp"
#include "protocols/arp.hpp"
#include "net_stack\network_stack.hpp"
#include <vector>
#include <cstdint>
#include <cstring> 
#include <iostream>
#include <array>


using namespace std;

uint16_t htons(uint16_t hostshort)
{
	uint16_t test = 1;
	if (*(reinterpret_cast<uint8_t *>(&test)) == 1)
	{
		// it is little endian, swap bytes

		return ((hostshort >> 8) | (hostshort << 8));
	}
	else {
		return hostshort;
	}
}



int main()
{
	net::NetworkConfig netconfig = {
		.mac_address = {0xF4,0x7B,0x09,0x51,0x91,0x63},
		.ipv4_address = {192,168,18,30},
		.gateway_address = {192,168,18,1}
	};

	if (hal_net_init(&netconfig,NetworkFiltering::ARP) != 0)
	{
		return 1;
	}
	hal_timer_init();

	net::NetworkStack stack(&netconfig);

    uint32_t last_arp_request_ms = 0;
    const uint32_t ARP_REQUEST_INTERVAL_MS = 2000; // 2 seconds

    NET_LOG_INFO(HAL, "Starting ARP discovery for gateway...");

    // This loop will run until we get a reply.
    while (!stack.is_gateway_mac_known()) {
        // Always poll for incoming packets and to run housekeeping.
        stack.poll();

        // Check if it's time to send our next ARP request.
        uint32_t current_time_ms = hal_timer_get_ms();
        if (current_time_ms - last_arp_request_ms > ARP_REQUEST_INTERVAL_MS) {
            stack.send_arp_request_for_gateway();
            last_arp_request_ms = current_time_ms;
        }
    }

    NET_LOG_INFO(HAL, "SUCCESS: Gateway MAC address has been resolved!");

    // We can now lookup the MAC and print it (optional)
    auto gateway_mac = stack.get_arp_cache().lookup(netconfig.gateway_address);
    if (gateway_mac) {
        // You would need to add a small function to print the MAC address here
    }

    NET_LOG_INFO(HAL, "Test complete. Shutting down.");
    hal_net_shutdown();

	return 0;
}
