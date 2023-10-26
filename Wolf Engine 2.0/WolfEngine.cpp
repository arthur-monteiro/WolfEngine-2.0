#include "WolfEngine.h"

#include <filesystem>

#include "Configuration.h"
#include "Dump.h"
#include "VulkanHelper.h"

Wolf::WolfEngine::WolfEngine(const WolfInstanceCreateInfo& createInfo)
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

	if(m_configuration->getUseRenderDoc())
	{
#ifdef _WIN32
		LoadLibrary(L"renderdoc.dll");
#else
		Debug::sendError("Can't open renderdoc dll");
#endif
	}

#ifndef __ANDROID__
	m_window.reset(new Window(createInfo.applicationName, m_configuration->getWindowWidth(), m_configuration->getWindowHeight(), this, windowResizeCallback));
#endif

#ifndef __ANDROID__
	m_vulkan.reset(new Vulkan(m_window->getWindow(), createInfo.useOVR));
#else
	m_vulkan.reset(new Vulkan(createInfo.androidWindow));
#endif
	g_vulkanInstance = m_vulkan.get();

#ifndef __ANDROID__
	m_swapChain.reset(new SwapChain(m_window->getWindow()));
#else
	m_swapChain.reset(new SwapChain(createInfo.androidWindow));
#endif

	m_gameContexts.resize(m_configuration->getMaxCachedFrames());

#ifndef __ANDROID__
	m_inputHandler.reset(new InputHandler(m_window->getWindow()));

	if (createInfo.htmlURL)
	{
		m_ultraLight.reset(new UltraLight(createInfo.htmlURL, createInfo.bindUltralightCallbacks, m_inputHandler.get()));
		m_ultraLight->waitInitializationDone();
	}
#endif
}

void Wolf::WolfEngine::initializePass(CommandRecordBase* pass) const
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

void Wolf::WolfEngine::updateEvents() const
{
#ifndef __ANDROID__
	m_window->pollEvents();
#endif

	if (m_cameraInterface)
	{
#ifndef __ANDROID__
		m_cameraInterface->update(m_window->getWindow());
#else
		m_cameraInterface->update();
#endif
	}

#ifndef __ANDROID__
	m_inputHandler->moveToNextFrame();
#endif
}

void Wolf::WolfEngine::frame(const std::span<CommandRecordBase*>& passes, const Semaphore* frameEndedSemaphore)
{
	if (passes.empty())
		Debug::sendError("No pass sent to frame");

#ifndef __ANDROID__
	if (!m_window->windowVisible())
		return;
#endif

	if (m_resizeIsNeeded)
	{
		vkDeviceWaitIdle(m_vulkan->getDevice());

		InitializationContext initializeContext;
		fillInitializeContext(initializeContext);

		if(m_resizeCallback)
		{
			m_resizeCallback(initializeContext.swapChainWidth, initializeContext.swapChainHeight);
		}

		for (CommandRecordBase* pass : passes)
		{
			pass->resize(initializeContext);
		}

		m_resizeIsNeeded = false;
	}

#ifndef __ANDROID__
	if (m_ultraLight)
	{
		m_ultraLight->processFrameJobs();
	}
#endif

	const uint32_t currentSwapChainImageIndex = m_swapChain->getCurrentImage(m_currentFrame);

	RecordContext recordContext{};
	recordContext.currentFrameIdx = m_currentFrame;
	recordContext.commandBufferIdx = m_currentFrame % g_configuration->getMaxCachedFrames();
	recordContext.swapChainImageIdx = currentSwapChainImageIndex;
	recordContext.swapchainImage = m_swapChain->getImage(currentSwapChainImageIndex);
#ifndef __ANDROID__
	recordContext.glfwWindow = m_window->getWindow();
#endif
	recordContext.camera = m_cameraInterface;
	recordContext.gameContext = m_gameContexts[recordContext.commandBufferIdx];

	for (CommandRecordBase* pass : passes)
	{
		pass->record(recordContext);
	}

	SubmitContext submitContext{};
	submitContext.currentFrameIdx = m_currentFrame;
	submitContext.commandBufferIdx = m_currentFrame % g_configuration->getMaxCachedFrames();
	submitContext.swapChainImageAvailableSemaphore = m_swapChain->getImageAvailableSemaphore(m_currentFrame % m_swapChain->getImageCount()); // we use the semaphore used to acquire image
#ifdef __ANDROID__
	submitContext.userInterfaceImageAvailableSemaphore = nullptr;
#else
	submitContext.userInterfaceImageAvailableSemaphore = m_ultraLight ? m_ultraLight->getImageCopySemaphore() : nullptr;
#endif
	submitContext.frameFence = m_swapChain->getFrameFence(m_currentFrame % g_configuration->getMaxCachedFrames());
	submitContext.device = m_vulkan->getDevice();

	for (CommandRecordBase* pass : passes)
	{
		pass->submit(submitContext);
	}

	m_swapChain->present(frameEndedSemaphore->getSemaphore(), currentSwapChainImageIndex);
	m_swapChain->synchroniseCPUFromGPU(m_currentFrame); // don't update UBs while a frame is being rendered

	m_currentFrame++;
}

void Wolf::WolfEngine::waitIdle() const
{
	vkDeviceWaitIdle(m_vulkan->getDevice());
}

#ifndef __ANDROID__
void Wolf::WolfEngine::getUserInterfaceJSObject(ultralight::JSObject& outObject) const
{
	m_ultraLight->getJSObject(outObject);
}

void Wolf::WolfEngine::evaluateUserInterfaceScript(const std::string& script) const
{
	m_ultraLight->requestScriptEvaluation(script);
}
#endif

void Wolf::WolfEngine::fillInitializeContext(InitializationContext& context) const
{
	context.swapChainWidth = m_swapChain->getImage(0)->getExtent().width;
	context.swapChainHeight = m_swapChain->getImage(0)->getExtent().height;
	context.swapChainFormat = m_swapChain->getImage(0)->getFormat();
	context.depthFormat = findDepthFormat(m_vulkan->getPhysicalDevice());
	context.swapChainImageCount = m_swapChain->getImageCount();
	for (uint32_t i = 0; i < context.swapChainImageCount; ++i)
		context.swapChainImages.push_back(m_swapChain->getImage(i));
#ifndef __ANDROID__
	if(m_ultraLight)
		context.userInterfaceImage = m_ultraLight->getImage();
#endif
	context.camera = m_cameraInterface;
}

void Wolf::WolfEngine::resize(int width, int height)
{
	vkDeviceWaitIdle(m_vulkan->getDevice());

	m_resizeIsNeeded = true;
#ifndef __ANDROID__
	m_swapChain->recreate(m_window->getWindow());
	if(m_ultraLight)
		m_ultraLight->resize(width, height);
#else
	m_swapChain->recreate();
#endif
}