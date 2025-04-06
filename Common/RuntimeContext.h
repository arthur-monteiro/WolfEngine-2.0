#pragma once
#include <cstdint>

namespace Wolf
{
	class RuntimeContext
	{
	public:
		RuntimeContext();

		void reset() { m_currentCPUFrameNumber = 0; }

		void incrementCPUFrameNumber() { m_currentCPUFrameNumber++; }
		//void incrementGPUFrameNumber() { m_currentGPUFrameNumber++; }

		uint32_t getCurrentCPUFrameNumber() const { return  m_currentCPUFrameNumber; }
		//uint32_t getCurrentGPUFrameNumber() const { return  m_currentGPUFrameNumber; }

	private:
		uint32_t m_currentCPUFrameNumber = 0;
		//uint32_t m_currentGPUFrameNumber = 0;
	};

	extern RuntimeContext* g_runtimeContext;
}
