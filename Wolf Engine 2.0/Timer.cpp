#include "Timer.h"

#include "Debug.h"

Wolf::Timer::Timer(std::string name) : m_name(std::move(name))
{
	m_startTimePoint = std::chrono::high_resolution_clock::now();
}

Wolf::Timer::~Timer()
{
	const long long millisecondsDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_startTimePoint).count();
	Debug::sendInfo("Timer " + m_name + " ended at " + std::to_string(millisecondsDuration / 1000.0f) + " second(s)");
}

void Wolf::Timer::updateCachedDuration()
{
	uint64_t duration = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_startTimePoint).count());
	m_elapsedTimeSinceLastUpdateInMs = duration - m_cachedMillisecondsDuration;
	m_cachedMillisecondsDuration = duration;
}
