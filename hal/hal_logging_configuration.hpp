#pragma once
#include "hal_logging.hpp" 

// This is the central configuration file for the logging system.
// By changing the #define values here and recompiling, you can control
// the verbosity of each component of the network stack independently.

// --- Step 1: Define the logging components ---
namespace net {
    enum class LogComponent {
        HAL,
        NET,
        ARP,
        IP,   // For the future
        ICMP, // For the future
        UDP,  // For the future
        TCP,  // For the future
        DHCP  // For the future
    };
} // namespace net



#define LOG_LEVEL_HAL LogLevel::INFO
#define LOG_LEVEL_NET LogLevel::DEBUG
#define LOG_LEVEL_ARP LogLevel::DEBUG

// Add future components here
// #define LOG_LEVEL_IP   net::LogLevel::INFO
// #define LOG_LEVEL_TCP  net::LogLevel::NONE
