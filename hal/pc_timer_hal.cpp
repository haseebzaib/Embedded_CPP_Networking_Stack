#include "hal_timer.hpp"
#include <chrono>

static auto start_time = std::chrono::steady_clock::now();
void hal_timer_init()
{
	start_time = std::chrono::steady_clock::now();
}

uint32_t hal_timer_get_ms()
{
	auto now = std::chrono::steady_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
}