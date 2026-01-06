#pragma once
#include <cstdint>

namespace Wolf
{
	class RuntimeContext
	{
	public:
		RuntimeContext();

		void reset() { m_currentCPUFrameNumber = FRAME_NUMBER_DEFAULT_VALUE; }

		void incrementCPUFrameNumber() { m_currentCPUFrameNumber++; }
		//void incrementGPUFrameNumber() { m_currentGPUFrameNumber++; }

		uint32_t getCurrentCPUFrameNumber() const { return  m_currentCPUFrameNumber; }
		//uint64_t getCurrentGPUFrameNumber() const { return  m_currentGPUFrameNumber; }

	private:
		static constexpr uint32_t FRAME_NUMBER_DEFAULT_VALUE = 0;

		uint32_t m_currentCPUFrameNumber = FRAME_NUMBER_DEFAULT_VALUE;
		//uint64_t m_currentGPUFrameNumber = 0;
	};

	extern RuntimeContext* g_runtimeContext;
}
