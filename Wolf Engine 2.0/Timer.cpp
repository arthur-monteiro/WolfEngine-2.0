#include "Timer.h"

#include "Debug.h"

Wolf::Timer::Timer(const std::string& name) : m_name(name)
{
	m_startTimePoint = std::chrono::high_resolution_clock::now();
}

Wolf::Timer::~Timer()
{
	const long long millisecondsDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_startTimePoint).count();
	Debug::sendInfo(m_name + " took " + std::to_string(millisecondsDuration / 1000.0f) + " second(s)");
}
