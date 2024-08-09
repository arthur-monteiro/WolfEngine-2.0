#include "WolfEngine.h"

#include <filesystem>
#include <limits>

#ifdef __ANDROID__
#include <android/native_window.h>
#include <android/native_window_jni.h>
#endif

#include "Configuration.h"
#include "Dump.h"
#include "ImageFileLoader.h"
#include "MipMapGenerator.h"

const Wolf::GraphicAPIManager* Wolf::g_graphicAPIManagerInstance = nullptr;

#ifndef __ANDROID__
VkExtent2D chooseExtent(GLFWwindow* window)
{
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	return VkExtent2D { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}
#endif

Wolf::WolfEngine::WolfEngine(const WolfInstanceCreateInfo& createInfo) : m_globalTimer("Wolf-Engine global timer"), m_renderMeshList(m_shaderList)
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
	m_graphicAPIManager.reset(GraphicAPIManager::instanciateGraphicAPIManager(m_window->getWindow(), createInfo.useOVR));
#else
	m_graphicAPIManager.reset(GraphicAPIManager::instanciateGraphicAPIManager(createInfo.androidWindow));
#endif
	g_graphicAPIManagerInstance = m_graphicAPIManager.get();

#ifdef __ANDROID__
	int32_t width = ANativeWindow_getWidth(createInfo.androidWindow);
	int32_t height = 2340; // ANativeWindow_getHeight(createInfo.androidWindow);
	VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
#else
	VkExtent2D extent = chooseExtent(m_window->getWindow());
#endif
	
	m_swapChain.reset(SwapChain::createSwapChain(extent));

	m_gameContexts.resize(m_configuration->getMaxCachedFrames());

#ifndef __ANDROID__
	m_inputHandler.reset(new InputHandler(m_window.createConstNonOwnerResource()));

	if (createInfo.htmlURL)
	{
		m_ultraLight.reset(new UltraLight(createInfo.htmlURL, createInfo.bindUltralightCallbacks, m_inputHandler.createNonOwnerResource()));
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
			const MipMapGenerator mipmapGenerator(imageFileLoader.getPixels(), { imageFileLoader.getWidth(), imageFileLoader.getHeight() }, VK_FORMAT_R8G8B8A8_SRGB);

			CreateImageInfo createImageInfo;
			createImageInfo.extent = { imageFileLoader.getWidth(), imageFileLoader.getHeight(), 1 };
			createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
			createImageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
			createImageInfo.mipLevelCount = mipmapGenerator.getMipLevelCount();
			createImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
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
	context.gameContext = m_gameContexts[m_currentFrame % g_configuration->getMaxCachedFrames()];
	context.frameIdx = m_currentFrame;
	context.swapChainExtent = m_swapChain->getImage(0)->getExtent();
	m_cameraList.moveToNextFrame(context);

	m_renderMeshList.moveToNextFrame(m_cameraList);
	m_shaderList.checkForModifiedShader();

	if (static_cast<bool>(m_materialsManager))
	{
		m_materialsManager->pushMaterialsToGPU();
	}

	m_globalTimer.updateCachedDuration();
}

void Wolf::WolfEngine::frame(const std::span<ResourceNonOwner<CommandRecordBase>>& passes, const Semaphore* frameEndedSemaphore)
{
	if (passes.empty())
		Debug::sendError("No pass sent to frame");

#ifndef __ANDROID__
	if (!m_window->windowVisible())
		return;
#endif

#ifndef __ANDROID__
	if (m_ultraLight)
	{
		m_ultraLight->processFrameJobs();
	}
#endif

	if (m_resizeIsNeeded)
	{
		waitIdle();

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

	const uint32_t currentSwapChainImageIndex = m_swapChain->getCurrentImage(m_currentFrame);

	RecordContext recordContext{};
	recordContext.currentFrameIdx = m_currentFrame;
	recordContext.commandBufferIdx = m_currentFrame % g_configuration->getMaxCachedFrames();
	recordContext.swapChainImageIdx = currentSwapChainImageIndex;
	recordContext.swapchainImage = m_swapChain->getImage(currentSwapChainImageIndex);
#ifndef __ANDROID__
	recordContext.glfwWindow = m_window->getWindow();
#endif
	recordContext.cameraList = &m_cameraList;
	recordContext.gameContext = m_gameContexts[recordContext.commandBufferIdx];
	recordContext.renderMeshList = &m_renderMeshList;
	if (m_materialsManager)
		recordContext.bindlessDescriptorSet = m_materialsManager->getDescriptorSet();

	for (const ResourceNonOwner<CommandRecordBase>& pass : passes)
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
	submitContext.graphicAPIManager = m_graphicAPIManager.get();

	for (const ResourceNonOwner<CommandRecordBase>& pass : passes)
	{
		pass->submit(submitContext);
	}

	m_swapChain->present(frameEndedSemaphore, currentSwapChainImageIndex);
	m_swapChain->synchroniseCPUFromGPU(m_currentFrame); // don't update UBs while a frame is being rendered

	m_currentFrame++;
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
#ifndef __ANDROID__
	if(m_ultraLight)
		context.userInterfaceImage = m_ultraLight->getImage();
#endif
}

void Wolf::WolfEngine::resize(int width, int height)
{
	waitIdle();

	m_resizeIsNeeded = true;
	m_swapChain->recreate({ static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
#ifndef __ANDROID__
	if(m_ultraLight)
		m_ultraLight->resize(width, height);
#endif
}