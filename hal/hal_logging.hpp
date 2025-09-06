#pragma once
#include <cstdint>
#include <string>

// Forward declare the component enum to avoid circular includes
namespace net {
    enum class LogComponent;
}


    enum class LogLevel {
        NONE = 0,
        ERROR = 1,
        WARN = 2,
        INFO = 3,
        DEBUG = 4,
    };
 
    // The underlying HAL function. This should NOT be called directly anymore.
    // The implementation is in pc_logging_hal.cpp or the equivalent MCU file.
    void hal_log(net::LogComponent component, LogLevel level, const char* fmt, ...);



#include "HAL_logging_configuration.hpp"

// A helper macro to get the component's max level
#define GET_LOG_LEVEL(component) LOG_LEVEL_##component

// The user-facing macros
#define NET_LOG(component, level, ...) \
    do { \
        if constexpr (level <= GET_LOG_LEVEL(component)) { \
            hal_log(net::LogComponent::component, level, __VA_ARGS__); \
        } \
    } while (0)

#define NET_LOG_ERROR(component, ...) NET_LOG(component, LogLevel::ERROR, __VA_ARGS__)
#define NET_LOG_WARN(component, ...)  NET_LOG(component, LogLevel::WARN,  __VA_ARGS__)
#define NET_LOG_INFO(component, ...)  NET_LOG(component, LogLevel::INFO,  __VA_ARGS__)
#define NET_LOG_DEBUG(component, ...) NET_LOG(component, LogLevel::DEBUG, __VA_ARGS__)

