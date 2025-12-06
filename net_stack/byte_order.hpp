#ifndef NET_STACK_BYTE_ORDER_HPP
#define NET_STACK_BYTE_ORDER_HPP

#include "stdint.h"

namespace net {


    /* Host to Network */
    inline uint16_t net_htons16(uint16_t v) {
        uint16_t test = 1;
        if (*reinterpret_cast<uint8_t *>(&test) == 1)
        {
            /*small-endian swap*/

            return static_cast<uint16_t>((v >> 8) | (v << 8));
        }
        else {
            /*big-endian no change*/
            return v;
        }
    }

    /* Network to Host */
    inline uint16_t net_ntohs16(uint16_t v)
    {
       return net_htons16(v);
    }

}


#endif