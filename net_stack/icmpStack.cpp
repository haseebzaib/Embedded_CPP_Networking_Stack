#include "icmpStack.hpp"
#include "network_stack.hpp"
#include "hal/hal_logging.hpp"
#include "hal/hal_timer.hpp"
#include "byte_order.hpp"
#include "cstring"
#include <span>



namespace net {


  void ICMP_stack::process_echo_request(NetworkStack& stack,std::span<const std::byte> frame)
  {



  }


  void ICMP_stack::process_icmp_packet(NetworkStack& stack,std::span<const std::byte> frame)
  {
      std::span<const std::byte> icmp_payload = frame;

      if(icmp_payload.size() < sizeof(IcmpHeader))
      {
        
        return;
      }

      const IcmpHeader *icmp_header = reinterpret_cast<const IcmpHeader *>(icmp_payload.data());

      uint16_t checksum = net_ntohs16(icmp_header->checksum);
      
      NET_LOG_DEBUG(ICMP, "Type|code|Checksum  %d|%d|%d",icmp_header->type,icmp_header->code,checksum);

      switch(static_cast<icmp_types>(icmp_header->type))
      {

        case icmp_types::EchoReply:
        {
          NET_LOG_DEBUG(ICMP, "EchoReply");

          break;
        }

        case icmp_types::EchoRequest:
        {

          NET_LOG_DEBUG(ICMP, "EchoRequest");
          process_echo_request(stack,icmp_payload);
          break;
        }



      }

      

  }


}