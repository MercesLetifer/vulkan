#pragma once

#include <chrono>
#include <string>

class Timer {
	std::chrono::steady_clock::time_point prev_;
	bool isWorking_ = false;

public:
	void start();
	bool isWorking() { return isWorking_; }
	bool tick();
};