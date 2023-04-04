#pragma once

#ifdef __ANDROID__
#include <android/asset_manager.h>
#endif
#include <string>

namespace Wolf
{
	class Configuration
	{
	public:
#ifndef __ANDROID__
		Configuration(const std::string& filename);
#else
		Configuration(const std::string& filename, AAssetManager* androidAssetManager);
#endif

		const uint32_t getWindowWidth() const { return m_windowWidth; }
		const uint32_t getWindowHeight() const { return m_windowHeight; }
		const uint32_t getMaxCachedFrames() const { return 2; }
		const bool getUseVIL() const { return m_useVIL; }

#ifdef __ANDROID__
		AAssetManager* getAndroidAssetManager() { return m_androidAssetManager; }
#endif

	private:
		uint32_t m_windowWidth = 0;
		uint32_t m_windowHeight = 0;
		uint32_t m_maxCachedFrames = 0;
		bool m_useVIL = false;

#ifdef __ANDROID__
		AAssetManager* m_androidAssetManager;
#endif
	};

	extern Configuration* g_configuration;
}

