#ifndef HAL_NETWORK_H
#define HAL_NETWORK_H

#include <cstddef>
#include <cstdint>
#include <array>

namespace net {
    // A structure to hold network configuration.
    // We'll populate this with MAC and IP addresses later.
    struct NetworkConfig {
        std::array<uint8_t, 6> mac_address;
        std::array<uint8_t, 4> ipv4_address;
        std::array<uint8_t, 4> gateway_address;
    };

}

/*Only usefull for testing on computers*/
enum class NetworkFiltering {
    ARP,
};

int hal_net_init(const net::NetworkConfig* config, NetworkFiltering Filtering);

/**
 * @brief Initializes the underlying network hardware/driver.
 * * @param config A pointer to the network configuration struct.
 * @return 0 on success, non-zero on failure.
 */
int hal_net_init(const net::NetworkConfig* config);

/**
 * @brief Sends a raw Ethernet frame onto the wire.
 * * @param data Pointer to the buffer containing the full frame (header + payload).
 * @param length The total length of the buffer in bytes.
 * @return 0 on success, non-zero on failure.
 */
int hal_net_send(const void* data, size_t length);

/**
 * @brief Attempts to receive a raw Ethernet frame from the wire.
 * * This should be a non-blocking function.
 * * @param buffer Pointer to a buffer where the received frame will be stored.
 * @param max_length The maximum size of the provided buffer.
 * @return The number of bytes received, or 0 if no frame was available.
 */
size_t hal_net_receive(void* buffer, size_t max_length);

/**
 * @brief Cleans up and deinitializes the network hardware/driver.
 */
void hal_net_shutdown();


#endif // HAL_NETWORK_H