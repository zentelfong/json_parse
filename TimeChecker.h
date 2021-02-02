#pragma once
#include <stdint.h>

#define ENABLE_TIMECHECKER 1

#if ENABLE_TIMECHECKER

#include<chrono>
using namespace std::chrono;

class TimeChecker {
public:
	TimeChecker() {
		m_begin = high_resolution_clock::now();
	}

	~TimeChecker() {

	}

	int64_t elapsed() const{
		return duration_cast<milliseconds>(
			high_resolution_clock::now() - m_begin).count();
	}

	int64_t elapsed_seconds() const{
		return duration_cast<seconds>(
			high_resolution_clock::now() - m_begin).count();
	}

	void reset() {
		m_begin = high_resolution_clock::now();
	}

private:
	time_point<high_resolution_clock> m_begin;
};

#else

class TimeChecker {
public:
	TimeChecker() {
	}

	~TimeChecker() {
	}

	int64_t elapsed() const {
		return 0;
	}

	int64_t elapsed_seconds() const {
		return 0;
	}

	void reset() {
	}
};

#endif
