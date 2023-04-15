#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <span>
#ifdef __ANDROID__
#include <android/asset_manager.h>
#endif

#include "CameraInterface.h"
#include "CommandRecordBase.h"
#include "Configuration.h"
#include "Debug.h"
#include "SwapChain.h"
#include "UltraLight.h"
#include "Vulkan.h"
#include "Window.h"

namespace Wolf
{
	struct WolfInstanceCreateInfo
	{
		uint32_t majorVersion;
		uint32_t minorVersion;
		std::string applicationName;

		std::string configFilename;

		const char* htmlURL = nullptr;

		bool useOVR = false;

		std::function<void(Debug::Severity, Debug::Type, const std::string&)> debugCallback;

#ifdef __ANDROID__
        ANativeWindow* androidWindow;
		AAssetManager* assetManager;
#endif
	};

	class WolfEngine
	{
	public:
		WolfEngine(const WolfInstanceCreateInfo& createInfo);

		void initializePass(CommandRecordBase* pass) const;

		bool windowShouldClose() const;
		void frame(const std::span<CommandRecordBase*>& passes, const Semaphore* frameEndedSemaphore);

		void waitIdle() const;

#ifndef __ANDROID__
		void getUserInterfaceJSObject(ultralight::JSObject& outObject) const;
#endif
		[[nodiscard]] const HardwareCapabilities& getHardwareCapabilities() const { return m_vulkan->getHardwareCapabilities(); }
		[[nodiscard]] uint32_t getCurrentFrame() const { return m_currentFrame; }

		void setCameraInterface(CameraInterface* cameraInterface) { m_cameraInterface = cameraInterface; }
		void setGameContexts(const std::vector<void*>& gameContexts) { m_gameContexts = gameContexts; }

	private:
		void fillInitializeContext(InitializationContext& context) const;

		static void windowResizeCallback(void* systemManagerInstance, int width, int height)
		{
			reinterpret_cast<WolfEngine*>(systemManagerInstance)->resize(width, height);
		}
		void resize(int width, int height);

	private:
		// Global instances
		std::unique_ptr<Configuration> m_configuration;
		std::unique_ptr<Vulkan> m_vulkan;
#ifndef __ANDROID__
		std::unique_ptr<Window> m_window;
#endif
		std::unique_ptr<SwapChain> m_swapChain;
#ifndef __ANDROID__
		std::unique_ptr<UltraLight> m_ultraLight;
		//std::unique_ptr<OVR> m_ovr;
#endif

		// Frame counter
		uint32_t m_currentFrame = 0;

		// Gameplay
		CameraInterface* m_cameraInterface = nullptr;
		std::vector<void*> m_gameContexts;

		bool m_resizeIsNeeded = false;
	};
}