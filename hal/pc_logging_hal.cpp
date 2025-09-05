#include "hal_logging.hpp"
#include <cstdarg> 
#include <cstdio>  


void hal_log(LogLevel level, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    // Add a prefix based on the log level
    switch (level) {
    case LogLevel::DEBUG:
        printf("[DEBUG] ");
        break;
    case LogLevel::INFO:
        printf("[INFO] ");
        break;
    case LogLevel::WARN:
        printf("[WARN] ");
        break;
    case LogLevel::ERROR:
        // Errors go to the standard error stream
        fprintf(stderr, "[ERROR] ");
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
        va_end(args);
        return; 
    }

    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}
