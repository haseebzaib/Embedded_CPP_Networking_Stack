#ifndef NET_STACK_BYTE_ORDER_HPP
#define NET_STACK_BYTE_ORDER_HPP

#include "stdint.h"
#include "type_traits"

namespace net
{

    template <typename T>
    union Split; // primary template

    template <>
    union Split<std::uint8_t>
    {
        std::uint8_t u8;
        struct
        {
            std::uint8_t low : 4;
            std::uint8_t high : 4;
        } le; // little endian

        /*Testing sometimes above approach does not work on all targets */
        std::uint8_t low() const { return u8 & 0x0F; }
        std::uint8_t high() const { return (u8 >> 4) & 0x0F; }
    };

    template <>
    union Split<std::uint16_t>
    {
        std::uint16_t u16;
        struct
        {
            std::uint8_t b0;
            std::uint8_t b1;
        } le; // little endian
    };

    template <>
    union Split<std::uint32_t>
    {
        std::uint32_t u32;
        struct
        {
            std::uint8_t b0;
            std::uint8_t b1;
            std::uint8_t b2;
            std::uint8_t b3;
        } le; // little endian
    };

    /* Host to Network */
    inline uint16_t net_htons16(uint16_t v)
    {
        uint16_t test = 1;
        if (*reinterpret_cast<uint8_t *>(&test) == 1)
        {
            /*small-endian swap*/

            return static_cast<uint16_t>((v >> 8) | (v << 8));
        }
        else
        {
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