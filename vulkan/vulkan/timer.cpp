#include "timer.h"

void Timer::start()
{
	if (isWorking_)
		return;

	prev_ = std::chrono::steady_clock::now();
	isWorking_ = true;
}

bool Timer::tick()
{
	if (isWorking_) {
		auto curr = std::chrono::steady_clock::now();
		auto duration_s = std::chrono::duration_cast<std::chrono::seconds>(curr - prev_);

		if (duration_s.count() >= 1) {
			prev_ = curr;
			return true;
		}
	}

	return false;
}