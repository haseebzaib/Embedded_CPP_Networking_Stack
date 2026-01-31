#ifndef NET_STACK_ICMP_H
#define NET_STACK_ICMP_H

#include "array"
#include "cstdint"
#include "optional"
#include <span>
#include "protocols/icmp.hpp"

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
     
    void process_icmp_packet(NetworkStack& stack,std::span<const std::byte> frame);
    private:
    void process_echo_request(NetworkStack& stack,std::span<const std::byte> frame);
    };

}

#endif