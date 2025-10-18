#include "WolfEngine.h"

#include <limits>

#include "SwapChain.h"
#include "SwapChain.h"

#ifdef __ANDROID__
#include <android/native_window.h>
#include <android/native_window_jni.h>
#endif

#include <Configuration.h>
#include <RuntimeContext.h>

#include "Dump.h"
#include "ImageFileLoader.h"
#include "MipMapGenerator.h"
#include "ProfilerCommon.h"

const Wolf::GraphicAPIManager* Wolf::g_graphicAPIManagerInstance = nullptr;

#ifndef __ANDROID__
Wolf::Extent2D chooseExtent(GLFWwindow* window)
{
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	return Wolf::Extent2D { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}
#endif

Wolf::WolfEngine::WolfEngine(const WolfInstanceCreateInfo& createInfo) : m_globalTimer("Wolf-Engine global timer")
{
#ifdef _WIN32
	SetUnhandledExceptionFilter(unhandledExceptionFilter);
#endif
	Debug::setCallback(createInfo.debugCallback);

	m_resizeCallback = createInfo.resizeCallback;

#ifndef __ANDROID__
	m_configuration.reset(new Configuration(createInfo.configFilename));
#else
    m_configuration.reset(new Configuration(createInfo.configFilename, createInfo.assetManager));
#endif
	g_configuration = m_configuration.get();

	m_runtimeContext.reset(new RuntimeContext());
	g_runtimeContext = m_runtimeContext.get();

	if(m_configuration->getUseRenderDoc())
	{
#ifdef _WIN32
		LoadLibraryW(L"renderdoc.dll");
#else
		Debug::sendError("Can't open renderdoc dll");
#endif
	}

#ifndef __ANDROID__
	m_window.reset(new Window(createInfo.applicationName, m_configuration->getWindowWidth(), m_configuration->getWindowHeight(), this, windowResizeCallback));
#endif

#ifndef __ANDROID__
	m_graphicAPIManager.reset(GraphicAPIManager::instanciateGraphicAPIManager(m_window->getWindow(), createInfo.useOVR));
#else
	m_graphicAPIManager.reset(GraphicAPIManager::instanciateGraphicAPIManager(createInfo.androidWindow));
#endif
	g_graphicAPIManagerInstance = m_graphicAPIManager.get();

#ifdef __ANDROID__
	int32_t width = ANativeWindow_getWidth(createInfo.androidWindow);
	int32_t height = 2340; // ANativeWindow_getHeight(createInfo.androidWindow);
	Extent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
#else
	Extent2D extent = chooseExtent(m_window->getWindow());
#endif

	SwapChain::SwapChainCreateInfo swapChainCreateInfo{};
	swapChainCreateInfo.extent = extent;

	switch (m_configuration->getColorSpace())
	{
		case Configuration::ColorSpace::SDR:
			swapChainCreateInfo.colorSpace = SwapChain::SwapChainCreateInfo::ColorSpace::S_RGB;
			swapChainCreateInfo.format = Format::R8G8B8A8_UNORM;
			break;
		case Configuration::ColorSpace::ESDR10:
			Debug::sendError("ESDR10 is not currently supported");
			swapChainCreateInfo.colorSpace = SwapChain::SwapChainCreateInfo::ColorSpace::S_RGB;
			swapChainCreateInfo.format = Format::R8G8B8A8_UNORM;
			break;
		case Configuration::ColorSpace::HDR16:
			swapChainCreateInfo.colorSpace = SwapChain::SwapChainCreateInfo::ColorSpace::LINEAR;
			swapChainCreateInfo.format = Format::R16G16B16A16_SFLOAT;
			break;
	};
	m_swapChain.reset(SwapChain::createSwapChain(swapChainCreateInfo));

	m_gameContexts.resize(m_configuration->getMaxCachedFrames());

#ifndef __ANDROID__
	m_inputHandler.reset(new InputHandler(m_window.createConstNonOwnerResource()));

	if (createInfo.htmlURL)
	{
		m_ultraLight.reset(new UltraLight(createInfo.htmlURL, createInfo.uiFinalLayout, createInfo.bindUltralightCallbacks, m_inputHandler.createNonOwnerResource()));
		m_ultraLight->waitInitializationDone();
	}
#endif

	if (createInfo.useBindlessDescriptor)
	{
		constexpr std::array defaultImageFilename = {
			"Textures/no_texture_albedo.png",
			"Textures/no_texture_normal.png",
			"Textures/no_texture_roughness_metalness_ao.png"
		};

		std::vector<DescriptorSetGenerator::ImageDescription> defaultImageDescription(defaultImageFilename.size());
		for (uint32_t i = 0; i < defaultImageFilename.size(); ++i)
		{
			const ImageFileLoader imageFileLoader(defaultImageFilename[i]);
			const MipMapGenerator mipmapGenerator(imageFileLoader.getPixels(), { imageFileLoader.getWidth(), imageFileLoader.getHeight() }, Format::R8G8B8A8_SRGB);

			CreateImageInfo createImageInfo;
			createImageInfo.extent = { imageFileLoader.getWidth(), imageFileLoader.getHeight(), 1 };
			createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
			createImageInfo.format = Format::R8G8B8A8_SRGB;
			createImageInfo.mipLevelCount = mipmapGenerator.getMipLevelCount();
			createImageInfo.usage = ImageUsageFlagBits::TRANSFER_DST | ImageUsageFlagBits::SAMPLED;
			m_defaultImages[i].reset(Image::createImage(createImageInfo));
			m_defaultImages[i]->copyCPUBuffer(imageFileLoader.getPixels(), Image::SampledInFragmentShader());

			for (uint32_t mipLevel = 1; mipLevel < mipmapGenerator.getMipLevelCount(); ++mipLevel)
			{
				m_defaultImages[i]->copyCPUBuffer(mipmapGenerator.getMipLevel(mipLevel).data(), Image::SampledInFragmentShader(mipLevel), mipLevel);
			}

			defaultImageDescription[i].imageView = m_defaultImages[i]->getDefaultImageView();
			defaultImageDescription[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		m_materialsManager.reset(new MaterialsGPUManager(defaultImageDescription));
	}

	m_lightManager.reset(new LightManager);
	m_renderMeshList.reset(new RenderMeshList(m_shaderList));
	m_physicsManager.reset(new Physics::PhysicsManager);

	m_multiThreadTaskManager.reset(new MultiThreadTaskManager);
	m_beforeFrameAndRecordThreadGroupId = m_multiThreadTaskManager->createThreadGroup(createInfo.threadCountBeforeFrameAndRecord, "Before frame and record");

	if (m_configuration->getForcedTimerMsPerFrame() > 0)
	{
		m_globalTimer.forceFixedTimerEachUpdate(m_configuration->getForcedTimerMsPerFrame());
	}
}

void Wolf::WolfEngine::initializePass(const ResourceNonOwner<CommandRecordBase>& pass) const
{
	InitializationContext context;
	fillInitializeContext(context);

	pass->initializeResources(context);
}

bool Wolf::WolfEngine::windowShouldClose() const
{
#ifndef __ANDROID__
	return m_window->windowShouldClose();
#else
	return false;
#endif
}

void Wolf::WolfEngine::updateBeforeFrame()
{
	PROFILE_FUNCTION

	m_multiThreadTaskManager->executeJobsForThreadGroup(m_beforeFrameAndRecordThreadGroupId);
	std::this_thread::sleep_for(std::chrono::microseconds(10)); // let some time to the threads to lock
	m_multiThreadTaskManager->waitForThreadGroup(m_beforeFrameAndRecordThreadGroupId);

	for (MultiThreadTaskManager::Job& job : m_jobsToExecuteAfterMTJobs)
	{
		job();
	}
	m_jobsToExecuteAfterMTJobs.clear();

	uint32_t currentFrame = g_runtimeContext->getCurrentCPUFrameNumber();

#ifndef __ANDROID__
	m_window->pollEvents();
#endif

#ifndef __ANDROID__
	m_inputHandler->moveToNextFrame();
#endif

	CameraUpdateContext context
#ifndef __ANDROID__
	(m_inputHandler.createConstNonOwnerResource());
#else
	{};
#endif
	context.frameIdx = currentFrame;
	context.swapChainExtent = m_swapChain->getImage(0)->getExtent();
	m_cameraList.moveToNextFrame(context);
	
	m_renderMeshList->moveToNextFrame();
	m_shaderList.checkForModifiedShader();

	if (static_cast<bool>(m_materialsManager))
	{
		m_materialsManager->updateBeforeFrame();
	}

	m_globalTimer.updateCachedDuration();

	m_lightManager->updateBeforeFrame();
}

uint32_t Wolf::WolfEngine::acquireNextSwapChainImage()
{
	if (m_previousFrameHasBeenReset)
	{
		return m_lastAcquiredSwapChainImageIndex;
	}

	uint32_t currentFrame = g_runtimeContext->getCurrentCPUFrameNumber();

	const uint32_t currentSwapChainImageIndex = m_swapChain->acquireNextImage(currentFrame);
	if (currentSwapChainImageIndex == SwapChain::NO_IMAGE_IDX)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(7)); // window is probably not visible, let the PC sleep a bit

		waitIdle();
		m_swapChain->resetAllFences();
		g_runtimeContext->reset();
	}

	m_lastAcquiredSwapChainImageIndex = currentSwapChainImageIndex;
	return currentSwapChainImageIndex;
}

void Wolf::WolfEngine::frame(const std::span<ResourceNonOwner<CommandRecordBase>>& passes, Semaphore* frameEndedSemaphore, uint32_t currentSwapChainImageIndex)
{
	PROFILE_FUNCTION

	uint32_t currentFrame = g_runtimeContext->getCurrentCPUFrameNumber();

	if (passes.empty())
		Debug::sendError("No pass sent to frame");

#ifndef __ANDROID__
	if (!m_window->windowVisible())
		return;
#endif

#ifndef __ANDROID__
	if (m_ultraLight && !m_previousFrameHasBeenReset)
	{
		m_ultraLight->processFrameJobs();
	}
#endif

	if (m_resizeIsNeeded)
	{
		waitIdle();
		m_swapChain->resetAllFences();
		g_runtimeContext->reset();
		currentFrame = g_runtimeContext->getCurrentCPUFrameNumber();

		InitializationContext initializeContext;
		fillInitializeContext(initializeContext);

		if (m_resizeCallback)
		{
			m_resizeCallback(initializeContext.swapChainWidth, initializeContext.swapChainHeight);
		}

		for (const ResourceNonOwner<CommandRecordBase>& pass : passes)
		{
			pass->resize(initializeContext);
		}

		m_resizeIsNeeded = false;
	}

	bool invalidateFrame = false;
	{
		PROFILE_SCOPED("Record GPU passes")

		RecordContext recordContext(m_lightManager.createNonOwnerResource(), m_renderMeshList.createNonOwnerResource());
		recordContext.currentFrameIdx = currentFrame;
		recordContext.swapChainImageIdx = currentSwapChainImageIndex;
		recordContext.swapchainImage = m_swapChain->getImage(currentSwapChainImageIndex);
#ifndef __ANDROID__
		recordContext.glfwWindow = m_window->getWindow();
#endif
		recordContext.cameraList = &m_cameraList;
		recordContext.gameContext = m_gameContexts[currentFrame % g_configuration->getMaxCachedFrames()];
		if (m_materialsManager)
			recordContext.bindlessDescriptorSet = m_materialsManager->getDescriptorSet();
		recordContext.globalTimer = &m_globalTimer;
		recordContext.graphicAPIManager = m_graphicAPIManager.get();
		recordContext.invalidateFrame = &invalidateFrame;

		for (const ResourceNonOwner<CommandRecordBase>& pass : passes)
		{
			pass->record(recordContext);
		}
	}

	if (invalidateFrame)
	{
		m_previousFrameHasBeenReset = true;
		return;
	}
	m_previousFrameHasBeenReset = false;

	{
		PROFILE_SCOPED("Submit GPU passes")

		SubmitContext submitContext{};
		submitContext.currentFrameIdx = currentFrame;
		submitContext.swapChainImageAvailableSemaphore = m_swapChain->getImageAvailableSemaphore(currentFrame % m_swapChain->getImageCount()); // we use the semaphore used to acquire image
#ifdef __ANDROID__
		submitContext.userInterfaceImageAvailableSemaphore = nullptr;
#else
		submitContext.userInterfaceImageAvailableSemaphore = m_ultraLight ? m_ultraLight->getImageCopySemaphore() : nullptr;
#endif
		submitContext.frameFence = m_swapChain->getFrameFence(currentFrame % g_configuration->getMaxCachedFrames());
		submitContext.graphicAPIManager = m_graphicAPIManager.get();
		submitContext.swapChainImageIndex = currentSwapChainImageIndex;

		for (const ResourceNonOwner<CommandRecordBase>& pass : passes)
		{
			pass->submit(submitContext);
		}
	}

	m_swapChain->present(frameEndedSemaphore, currentSwapChainImageIndex);

	if (currentFrame >= g_configuration->getMaxCachedFrames() - 1)
	{
		PROFILE_SCOPED("Wait for GPU frame to end")
		m_swapChain->synchroniseCPUFromGPU(currentFrame + g_configuration->getMaxCachedFrames() - 1); // don't update UBs while a frame is being rendered
	}

	g_runtimeContext->incrementCPUFrameNumber();
}

void Wolf::WolfEngine::addJobBeforeFrame(const MultiThreadTaskManager::Job& job, bool runAfterAllJobs)
{
	if (runAfterAllJobs)
	{
		m_jobsToExecuteAfterMTJobs.emplace_back(job);
	}
	else
	{
		m_multiThreadTaskManager->addJobToThreadGroup(m_beforeFrameAndRecordThreadGroupId, job);
	}
}

void Wolf::WolfEngine::waitIdle() const
{
	m_graphicAPIManager->waitIdle();
}

#ifndef __ANDROID__
void Wolf::WolfEngine::getUserInterfaceJSObject(ultralight::JSObject& outObject) const
{
	m_ultraLight->getJSObject(outObject);
}

void Wolf::WolfEngine::evaluateUserInterfaceScript(const std::string& script)
{
	if (m_configuration->getUICommands())
	{
		m_savedUICommands.push_back(script);
	}
	m_ultraLight->requestScriptEvaluation(script);
}
#endif

void Wolf::WolfEngine::fillInitializeContext(InitializationContext& context) const
{
	context.swapChainWidth = m_swapChain->getImage(0)->getExtent().width;
	context.swapChainHeight = m_swapChain->getImage(0)->getExtent().height;
	context.swapChainFormat = m_swapChain->getImage(0)->getFormat();
	context.depthFormat = m_graphicAPIManager->getDepthFormat();
	context.swapChainImageCount = m_swapChain->getImageCount();
	for (uint32_t i = 0; i < context.swapChainImageCount; ++i)
		context.swapChainImages.push_back(m_swapChain->getImage(i));
	context.swapChainColorSpace = m_swapChain->getColorSpace();
#ifndef __ANDROID__
	if(m_ultraLight)
		context.userInterfaceImage = m_ultraLight->getImage();
#endif
}

void Wolf::WolfEngine::resize(int width, int height)
{
	waitIdle();

	m_resizeIsNeeded = true;
	m_swapChain->recreate({ static_cast<uint32_t>(width), static_cast<uint32_t>(height) });
#ifndef __ANDROID__
	if(m_ultraLight)
		m_ultraLight->resize(width, height);
#endif

	m_materialsManager->resize({ static_cast<uint32_t>(width), static_cast<uint32_t>(height) });
}