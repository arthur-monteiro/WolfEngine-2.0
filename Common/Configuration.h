#pragma once

#ifdef __ANDROID__
#include <android/asset_manager.h>
#endif
#include <cstdint>
#include <string>

namespace Wolf
{
	class Configuration
	{
	public:
#ifndef __ANDROID__
		Configuration(const std::string& filePath);
#else
		Configuration(const std::string& filePath, AAssetManager* androidAssetManager);
#endif

		[[nodiscard]] uint32_t getWindowWidth() const { return m_windowWidth; }
		[[nodiscard]] uint32_t getWindowHeight() const { return m_windowHeight; }
		[[nodiscard]] uint32_t getMaxCachedFrames() const { return m_maxCachedFrames; }
		[[nodiscard]] bool getUseVIL() const { return m_useVIL; }
		[[nodiscard]] bool getUseRenderDoc() const { return m_useRenderDoc; }
		[[nodiscard]] bool getUICommands() const { return m_saveUICommands; }
		[[nodiscard]] bool getEnableGPUDebugMarkers() const { return m_enableGPUDebugMarkers; }
		[[nodiscard]] bool getUseVirtualTexture() const { return m_useVirtualTexture; }

#ifdef __ANDROID__
		AAssetManager* getAndroidAssetManager() const { return m_androidAssetManager; }
#endif

	private:
		uint32_t m_windowWidth = 0;
		uint32_t m_windowHeight = 0;
		uint32_t m_maxCachedFrames = 1;
		bool m_useVIL = false;
		bool m_useRenderDoc = false;
		bool m_saveUICommands = false;
		bool m_enableGPUDebugMarkers = false;
		bool m_useVirtualTexture = false;

#ifdef __ANDROID__
		AAssetManager* m_androidAssetManager;
#endif
	};

	extern const Configuration* g_configuration;
}

