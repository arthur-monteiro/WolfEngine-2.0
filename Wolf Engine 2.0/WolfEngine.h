#pragma once

#include <functional>
#include <memory>
#include <span>
#ifdef __ANDROID__
#include <android/asset_manager.h>
#endif

// Common
#include <Debug.h>

// API Broker
#include <GraphicAPIManager.h>

#include "CameraList.h"
#include "CommandRecordBase.h"
#include "Configuration.h"
#include "InputHandler.h"
#include "LightManager.h"
#include "MaterialsGPUManager.h"
#include "MultiThreadTaskManager.h"
#include "PhysicsManager.h"
#include "RenderMeshList.h"
#include "ResourceUniqueOwner.h"
#include "RuntimeContext.h"
#include "ShaderList.h"
#include "SwapChain.h"
#include "UltraLight.h"
#include "Window.h"
#include "Timer.h"

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

		uint32_t threadCountBeforeFrameAndRecord = 1;

#ifdef __ANDROID__
        ANativeWindow* androidWindow;
		AAssetManager* assetManager;
#endif
	};

	class WolfEngine
	{
	public:
		WolfEngine(const WolfInstanceCreateInfo& createInfo);

		void initializePass(const ResourceNonOwner<CommandRecordBase>& pass) const;

		bool windowShouldClose() const;
		void updateBeforeFrame();
		uint32_t acquireNextSwapChainImage();
		void frame(const std::span<ResourceNonOwner<CommandRecordBase>>& passes, Semaphore* frameEndedSemaphore, uint32_t currentSwapChainImageIndex);

		void addJobBeforeFrame(const MultiThreadTaskManager::Job& job, bool runAfterAllJobs = false);

		void waitIdle() const;

#ifndef __ANDROID__
		void getUserInterfaceJSObject(ultralight::JSObject& outObject) const;
		void evaluateUserInterfaceScript(const std::string& script);
		[[nodiscard]] ResourceNonOwner<InputHandler> getInputHandler() { return m_inputHandler.createNonOwnerResource(); }
#endif
		[[nodiscard]] Extent3D getSwapChainExtent() const { return m_swapChain->getImage(0)->getExtent(); }

		[[nodiscard]] bool isRayTracingAvailable() const { return m_graphicAPIManager->isRayTracingAvailable(); }

		void setGameContexts(const std::vector<void*>& gameContexts) { m_gameContexts = gameContexts; }

		[[nodiscard]] CameraList& getCameraList() { return m_cameraList; }
		[[nodiscard]] ResourceNonOwner<RenderMeshList> getRenderMeshList() { return m_renderMeshList.createNonOwnerResource(); }
		[[nodiscard]] ResourceUniqueOwner<LightManager>& getLightManager() { return m_lightManager; }
		[[nodiscard]] ResourceNonOwner<MaterialsGPUManager> getMaterialsManager() { return m_materialsManager.createNonOwnerResource(); }
		[[nodiscard]] const std::vector<std::string>& getSavedUICommands() const { return m_savedUICommands; }
		[[nodiscard]] const Timer& getGlobalTimer() const { return m_globalTimer; }
		[[nodiscard]] ResourceNonOwner<Physics::PhysicsManager> getPhysicsManager() { return m_physicsManager.createNonOwnerResource(); }

	private:
		void fillInitializeContext(InitializationContext& context) const;

		static void windowResizeCallback(void* systemManagerInstance, int width, int height)
		{
			static_cast<WolfEngine*>(systemManagerInstance)->resize(width, height);
		}
		void resize(int width, int height);

	private:
		std::unique_ptr<Configuration> m_configuration;
		std::unique_ptr<RuntimeContext> m_runtimeContext;
		std::unique_ptr<GraphicAPIManager> m_graphicAPIManager;
#ifndef __ANDROID__
		ResourceUniqueOwner<Window> m_window;
#endif
		ResourceUniqueOwner<SwapChain> m_swapChain;
#ifndef __ANDROID__
		ResourceUniqueOwner<InputHandler> m_inputHandler;
		ResourceUniqueOwner<UltraLight> m_ultraLight;
		//std::unique_ptr<OVR> m_ovr;
#endif
		ResourceUniqueOwner<Physics::PhysicsManager> m_physicsManager;

		// Timer
		Timer m_globalTimer;

		// Gameplay
		CameraList m_cameraList;
		std::vector<void*> m_gameContexts;
		ResourceUniqueOwner<LightManager> m_lightManager;

		bool m_resizeIsNeeded = false;
		std::function<void(uint32_t, uint32_t)> m_resizeCallback;

		// Mesh render
		ResourceUniqueOwner<RenderMeshList> m_renderMeshList;
		ShaderList m_shaderList;
		std::array<std::unique_ptr<Image>, 5> m_defaultImages;
		ResourceUniqueOwner<MaterialsGPUManager> m_materialsManager;

		// Saves
		std::vector<std::string> m_savedUICommands;

		// Job executions
		ResourceUniqueOwner<MultiThreadTaskManager> m_multiThreadTaskManager;
		MultiThreadTaskManager::ThreadGroupId m_beforeFrameAndRecordThreadGroupId;
		std::vector<MultiThreadTaskManager::Job> m_jobsToExecuteAfterMTJobs;
	};

	extern const GraphicAPIManager* g_graphicAPIManagerInstance;
}