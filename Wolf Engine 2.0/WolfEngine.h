#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <span>

#include "CameraInterface.h"
#include "Configuration.h"
#include "Debug.h"
#include "PassBase.h"
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

		const char* htmlStringUI = nullptr;

		bool useOVR = false;

		std::function<void(Debug::Severity, Debug::Type, const std::string&)> debugCallback;
	};

	class WolfEngine
	{
	public:
		WolfEngine(const WolfInstanceCreateInfo& createInfo);

		void initializePass(PassBase* pass);

		bool windowShouldClose();
		void frame(const std::span<PassBase*>& passes, const Semaphore* frameEndedSemaphore);

		void waitIdle();

		void getUserInterfaceJSObject(ultralight::JSObject& outObject);

		void setCameraInterface(CameraInterface* cameraInterface) { m_cameraInterface = cameraInterface; }

	private:
		void fillInitializeContext(InitializationContext& context);

		static void windowResizeCallback(void* systemManagerInstance, int width, int height)
		{
			reinterpret_cast<WolfEngine*>(systemManagerInstance)->resize(width, height);
		}
		void resize(int width, int height);

	private:
		// Global instances
		std::unique_ptr<Configuration> m_configuration;
		std::unique_ptr<Vulkan> m_vulkan;
		std::unique_ptr<Window> m_window;
		std::unique_ptr<SwapChain> m_swapChain;
		std::unique_ptr<UltraLight> m_ultraLight;
		//std::unique_ptr<OVR> m_ovr;

		// Frame counter
		uint32_t m_currentFrame = 0;

		// Gameplay
		CameraInterface* m_cameraInterface = nullptr;

		bool m_resizeIsNeeded = false;
	};
}