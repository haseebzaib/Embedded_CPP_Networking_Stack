#ifndef NET_STACK_BYTE_ORDER_HPP
#define NET_STACK_BYTE_ORDER_HPP

#include "stdint.h"
#include "type_traits"
#include <limits>

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

    template <typename integer_type, unsigned Offset, unsigned Width>
    constexpr integer_type extract_bits(integer_type v)
    {
        static_assert(std::is_integral_v<integer_type>, "T must be an integral type");
        static_assert(std::is_unsigned_v<integer_type>, "T muist be unsigned");

        constexpr unsigned Bits = std::numeric_limits<integer_type>::digits;
        static_assert(Width >= 1, "Width must be >= 1");
        static_assert(Offset < Bits, "Offset out of range");
        static_assert(Offset + Width <= Bits, "Offset + Width out of range");

        // mask = (1<<Width)-1 but handle Width==Bits safely
        constexpr integer_type mask = (Width == Bits)
                               ? ~integer_type{0}
                               : (integer_type{1} << Width) - integer_type{1};

        return (v >> Offset) & mask;
    }

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