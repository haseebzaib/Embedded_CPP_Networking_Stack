#include "hal/hal_logging.hpp"
#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <map>



    // Helper to convert component enum to string for printing
    static const std::map<net::LogComponent, std::string> component_names = {
        {net::LogComponent::HAL, "HAL"},
        {net::LogComponent::NET, "NET"},
        {net::LogComponent::ARP, "ARP"},
        {net::LogComponent::IP, "IP"},
        {net::LogComponent::ICMP, "ICMP"},
        {net::LogComponent::UDP, "UDP"},
        {net::LogComponent::TCP, "TCP"},
        {net::LogComponent::DHCP, "DHCP"},
    };

    // Helper to convert level enum to string for printing
    static const std::map<LogLevel, std::string> level_names = {
        {LogLevel::NONE,  "NONE"},
        {LogLevel::ERROR, "ERROR"},
        {LogLevel::WARN,  "WARN"},
        {LogLevel::INFO,  "INFO"},
        {LogLevel::DEBUG, "DEBUG"},
    };
    void hal_log(net::LogComponent component, LogLevel level, const char* fmt, ...) {
        // --- FIX #2: Immediately return if the level is NONE ---
        // This is the most important part of the fix.
        if (level == LogLevel::NONE) {
            return;
        }

        // A global minimum log level. For PC, we print everything.
        constexpr LogLevel global_min_level = LogLevel::DEBUG;
        if (level > global_min_level) {
            return;
        }

        // Build the log prefix, e.g., "[ARP] [DEBUG] "
        // This line will no longer crash because we added the NONE entry.
        std::string prefix = "[" + component_names.at(component) + "] [" + level_names.at(level) + "] ";

        // Use vsnprintf to handle the variable arguments safely
        char buffer[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        // Print to the appropriate stream
        if (level == LogLevel::ERROR) {
            std::cerr << prefix << buffer << std::endl;
        }
        else {
            std::cout << prefix << buffer << std::endl;
        }
    }




