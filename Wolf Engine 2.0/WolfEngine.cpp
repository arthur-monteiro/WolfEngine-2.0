#include "WolfEngine.h"

#include "VulkanHelper.h"

Wolf::WolfEngine::WolfEngine(const WolfInstanceCreateInfo& createInfo)
{
	Debug::setCallback(createInfo.debugCallback);

	m_configuration.reset(new Configuration(createInfo.configFilename));
	g_configuration = m_configuration.get();

	m_window.reset(new Window(createInfo.applicationName, m_configuration->getWindowWidth(), m_configuration->getWindowHeight(), this, windowResizeCallback));

	m_vulkan.reset(new Vulkan(m_window->getWindow(), createInfo.useOVR));
	g_vulkanInstance = m_vulkan.get();

	m_swapChain.reset(new SwapChain(m_window->getWindow()));

	if(createInfo.htmlStringUI)
		m_ultraLight.reset(new UltraLight(m_configuration->getWindowWidth(), m_configuration->getWindowHeight(), createInfo.htmlStringUI));
}

void Wolf::WolfEngine::initializePass(PassBase* pass)
{
	InitializationContext context;
	fillInitializeContext(context);

	pass->initializeResources(context);
}

bool Wolf::WolfEngine::windowShouldClose()
{
	return m_window->windowShouldClose();
}

void Wolf::WolfEngine::frame(const std::span<PassBase*>& passes, const Semaphore* frameEndedSemaphore)
{
	if (passes.empty())
		Debug::sendError("No pass sent to frame");

	m_window->pollEvents();
	if (!m_window->windowVisible())
		return;

	if (m_resizeIsNeeded)
	{
		vkDeviceWaitIdle(m_vulkan->getDevice());

		InitializationContext initializeContext;
		fillInitializeContext(initializeContext);

		for (PassBase* pass : passes)
		{
			pass->resize(initializeContext);
		}

		m_resizeIsNeeded = false;
	}

	if (m_ultraLight)
	{
		m_ultraLight->update(m_window->getWindow());
		m_ultraLight->render();
	}

	m_swapChain->synchroniseCPUFromGPU(m_currentFrame);
	uint32_t currentSwapChainImageIndex = m_swapChain->getCurrentImage(m_currentFrame);

	RecordContext recordContext;
	recordContext.currentFrameIdx = m_currentFrame;
	recordContext.commandBufferIdx = m_currentFrame % g_configuration->getMaxCachedFrames();
	recordContext.swapChainImageIdx = currentSwapChainImageIndex;
	recordContext.swapchainImage = m_swapChain->getImage(currentSwapChainImageIndex);

	for (PassBase* pass : passes)
	{
		pass->record(recordContext);
	}

	SubmitContext submitContext;
	submitContext.currentFrameIdx = m_currentFrame;
	submitContext.commandBufferIdx = m_currentFrame % g_configuration->getMaxCachedFrames();
	submitContext.imageAvailableSemaphore = m_swapChain->getImageAvailableSemaphore(m_currentFrame % m_swapChain->getImageCount()); // we use the semaphore used to acquire image
	submitContext.frameFence = m_swapChain->getFrameFence(m_currentFrame % g_configuration->getMaxCachedFrames());
	submitContext.device = m_vulkan->getDevice();

	for (PassBase* pass : passes)
	{
		pass->submit(submitContext);
	}

	m_swapChain->present(frameEndedSemaphore->getSemaphore(), currentSwapChainImageIndex);

	m_currentFrame++;
}

void Wolf::WolfEngine::waitIdle()
{
	vkDeviceWaitIdle(m_vulkan->getDevice());
}

void Wolf::WolfEngine::getUserInterfaceJSObject(ultralight::JSObject& outObject)
{
	m_ultraLight->getJSObject(outObject);
}

void Wolf::WolfEngine::fillInitializeContext(InitializationContext& context)
{
	context.swapChainWidth = m_swapChain->getImage(0)->getExtent().width;
	context.swapChainHeight = m_swapChain->getImage(0)->getExtent().height;
	context.swapChainFormat = m_swapChain->getImage(0)->getFormat();
	context.depthFormat = findDepthFormat(m_vulkan->getPhysicalDevice());
	context.swapChainImageCount = m_swapChain->getImageCount();
	for (uint32_t i = 0; i < context.swapChainImageCount; ++i)
		context.swapChainImages.push_back(m_swapChain->getImage(i));
	if(m_ultraLight)
		context.userInterfaceImage = m_ultraLight->getImage();
}

void Wolf::WolfEngine::resize(int width, int height)
{
	vkDeviceWaitIdle(m_vulkan->getDevice());

	m_resizeIsNeeded = true;
	m_swapChain->recreate(m_window->getWindow());
	m_ultraLight->resize(width, height);
}
