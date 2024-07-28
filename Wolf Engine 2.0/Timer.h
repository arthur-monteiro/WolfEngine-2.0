#pragma once

#include <chrono>
#include <string>

namespace Wolf
{
	class Timer
	{
	public:
		Timer(std::string name);
		Timer(const Timer&) = delete;

		~Timer();

		void updateCachedDuration();

		uint64_t getCurrentCachedMillisecondsDuration() const { return m_cachedMillisecondsDuration; }
		uint64_t getElapsedTimeSinceLastUpdateInMs() const { return m_elapsedTimeSinceLastUpdateInMs; }

	private:
		std::chrono::steady_clock::time_point m_startTimePoint;
		std::string m_name;

		uint64_t m_cachedMillisecondsDuration = 0;
		uint64_t m_elapsedTimeSinceLastUpdateInMs = 0;
	};
}
