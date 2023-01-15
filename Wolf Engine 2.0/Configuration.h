#pragma once

#include <string>

namespace Wolf
{
	class Configuration
	{
	public:
		Configuration(const std::string& filename);

		const uint32_t getWindowWidth() const { return m_windowWidth; }
		const uint32_t getWindowHeight() const { return m_windowHeight; }
		const uint32_t getMaxCachedFrames() const { return m_maxCachedFrames; }

	private:
		uint32_t m_windowWidth = 0;
		uint32_t m_windowHeight = 0;
		uint32_t m_maxCachedFrames = 0;
	};

	extern Configuration* g_configuration;
}

