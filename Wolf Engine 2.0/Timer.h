#pragma once

#include <chrono>
#include <string>

namespace Wolf
{
	class Timer
	{
	public:
		Timer(const std::string& name);
		Timer(const Timer&) = delete;

		~Timer();

	private:
		std::chrono::steady_clock::time_point m_startTimePoint;
		std::string m_name;
	};
}
