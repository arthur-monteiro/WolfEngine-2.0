#pragma once

#include <functional>
#include <memory>
#include <span>
#ifdef __ANDROID__
#include <android/asset_manager.h>
#endif

#include "BindlessDescriptor.h"
#include "CameraList.h"
#include "CommandRecordBase.h"
#include "Configuration.h"
#include "Debug.h"
#include "InputHandler.h"
#include "RenderMeshList.h"
#include "ShaderList.h"
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
		std::function<void()> bindUltralightCallbacks;

		bool useOVR = false;
		bool useBindlessDescriptor = false;

		std::function<void(Debug::Severity, Debug::Type, const std::string&)> debugCallback;
		std::function<void(uint32_t, uint32_t)> resizeCallback;

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
		void updateEvents();
		void frame(const std::span<CommandRecordBase*>& passes, const Semaphore* frameEndedSemaphore);

		void waitIdle() const;

#ifndef __ANDROID__
		void getUserInterfaceJSObject(ultralight::JSObject& outObject) const;
		void evaluateUserInterfaceScript(const std::string& script) const;
#endif
		[[nodiscard]] uint32_t getCurrentFrame() const { return m_currentFrame; }
#ifndef __ANDROID__
		[[nodiscard]] const InputHandler& getInputHandler() const { return *m_inputHandler; }
#endif
		VkExtent3D getSwapChainExtent() const { return m_swapChain->getImage(0)->getExtent(); }

		bool isRayTracingAvailable() const { return m_vulkan->isRayTracingAvailable(); }

		void setGameContexts(const std::vector<void*>& gameContexts) { m_gameContexts = gameContexts; }

		CameraList& getCameraList() { return m_cameraList; }
		RenderMeshList& getRenderMeshList() { return m_renderMeshList; }
		BindlessDescriptor& getBindlessDescriptor() const { return *m_bindlessDescriptor; }

	private:
		void fillInitializeContext(InitializationContext& context) const;

		static void windowResizeCallback(void* systemManagerInstance, int width, int height)
		{
			static_cast<WolfEngine*>(systemManagerInstance)->resize(width, height);
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
		CameraList m_cameraList;
		std::vector<void*> m_gameContexts;
#ifndef __ANDROID__
		std::unique_ptr<InputHandler> m_inputHandler;
#endif

		bool m_resizeIsNeeded = false;
		std::function<void(uint32_t, uint32_t)> m_resizeCallback;

		// Mesh render
		RenderMeshList m_renderMeshList;
		ShaderList m_shaderList;
		std::unique_ptr<BindlessDescriptor> m_bindlessDescriptor;
		std::array<std::unique_ptr<Image>, 5> m_defaultImages;
	};
}