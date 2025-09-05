#ifndef HAL_TIMER_H
#define HAL_TIMER_H

#include <cstdint>



void hal_timer_init();

/**
 * @brief Gets a monotonically increasing millisecond tick count.
 * * This should be used for measuring time intervals and should not be affected
 * by changes to the system's wall-clock time.
 * * @return The number of milliseconds since an arbitrary start point (e.g., boot-up).
 */
uint32_t hal_timer_get_ms();

#endif // HAL_TIMER_H
