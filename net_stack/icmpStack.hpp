#ifndef NET_STACK_ICMP_H
#define NET_STACK_ICMP_H

#include "array"
#include "cstdint"
#include "optional"
#include <span>
#include "protocols/icmp.hpp"
#include "protocols/ethernet.hpp"
#include "protocols/ipv4.hpp"

// forward declaration to avoid circular dependencies
namespace net
{
    class NetworkStack;
}

namespace net
{

    class ICMP_stack
    {

    public:
        void process_icmp_packet(NetworkStack &stack,const EthernetHeader &rx_eth, const Ipv4Header &rx_ip , std::span<const std::byte> frame);

    private:
        void process_echo_request(NetworkStack &stack,const EthernetHeader &rx_eth, const Ipv4Header &rx_ip ,  std::span<const std::byte> frame);
        uint16_t internet_checksum(std::span<const std::byte> bytes);
    
    
    };

}

#endif