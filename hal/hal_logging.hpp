#ifndef HAL_LOGGING_H
#define HAL_LOGGING_H

// Defines the severity of the log message.
enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

// The platform must implement this function.
// It uses a printf-style format for convenience.
void hal_log(LogLevel level, const char* fmt, ...);

#endif // HAL_LOGGING_H
