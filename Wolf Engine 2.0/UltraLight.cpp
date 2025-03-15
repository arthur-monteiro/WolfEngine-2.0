#include "UltraLight.h"

#include <chrono>
#include <iostream>
#include <thread>

#include "Configuration.h"
#include "Debug.h"
#include "InputHandler.h"
#include "Semaphore.h"
#include "Timer.h"
#include "Window.h"

using namespace ultralight;

Wolf::UltraLight::UltraLight(const char* htmlURL, const std::function<void()>& bindCallbacks, const ResourceNonOwner<InputHandler>& inputHandler) : m_inputHandler(inputHandler)
{
    m_thread = std::thread(&UltraLight::processImplementation, this, htmlURL, bindCallbacks);
}

Wolf::UltraLight::~UltraLight()
{
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_needUpdate = true;
        m_stopThreadRequested = true;
    }
    m_updateCondition.notify_all();
    m_thread.join();
}

void Wolf::UltraLight::waitInitializationDone() const
{
    while (!m_ultraLightImplementation)
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void Wolf::UltraLight::processFrameJobs()
{
    m_inputHandler->lockCache(m_ultraLightImplementation.get());
    m_inputHandler->pushDataToCache(m_ultraLightImplementation.get());
    m_inputHandler->unlockCache(m_ultraLightImplementation.get());

    if (m_mutex.try_lock())
    {
        m_ultraLightImplementation->submitCopyImageCommandBuffer();

        m_mutex.unlock();
        m_needUpdate = true;
        m_updateCondition.notify_all();

        m_copySubmittedThisFrame = true;
    }
    else
    {
        m_copySubmittedThisFrame = false;
    }
}

void Wolf::UltraLight::requestScriptEvaluation(const std::string& script)
{
    m_evaluateScriptRequestsMutex.lock();
    m_evaluateScriptRequests.push_back(script);
    m_evaluateScriptRequestsMutex.unlock();
}

void Wolf::UltraLight::resize(uint32_t width, uint32_t height)
{
    m_resizeRequest = { width, height };

    {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_needUpdate = true;
        m_updateCondition.notify_all();
    }

    while (m_resizeRequest.width != 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

void Wolf::UltraLight::processImplementation(const char* htmlURL, const std::function<void()>& bindCallbacks)
{
#ifdef _WIN32
    SetThreadDescription(GetCurrentThread(), L"UltraLight - Update");
#endif 

    std::string currentPath = std::filesystem::current_path().string();
    std::ranges::replace(currentPath, '\\', '/');
    std::string escapedCurrentPath;
    for (const char currentPathChar : currentPath)
    {
        if (currentPathChar == ' ')
            escapedCurrentPath += "%20";
        else
            escapedCurrentPath += currentPathChar;
    }
    const std::string absoluteURL = "file:///" + escapedCurrentPath + "/" + htmlURL;
    m_ultraLightImplementation.reset(new UltraLightImplementation(g_configuration->getWindowWidth(), g_configuration->getWindowHeight(), absoluteURL, htmlURL, m_inputHandler));

    m_bindUltralightCallbacks = bindCallbacks;

    for (;;)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_updateCondition.wait(lock, [&] { return m_needUpdate; });

        if (m_stopThreadRequested)
        {
            m_ultraLightImplementation.reset();
            return;
        }

        if (m_resizeRequest.width != 0)
        {
            m_ultraLightImplementation->resize(m_resizeRequest.width, m_resizeRequest.height);
            m_ultraLightImplementation->createOutputAndRecordCopyCommandBuffer(m_resizeRequest.width, m_resizeRequest.height);
            m_resizeRequest = { 0, 0 };
            m_needUpdate = false;
            continue;
        }

        if (m_ultraLightImplementation->reloadIfModified())
        {
            bindCallbacks();
            m_needUpdate = false;
            continue;
        }

        m_ultraLightImplementation->evaluateScript("frame()");

        m_evaluateScriptRequestsMutex.lock();
        if (!m_evaluateScriptRequests.empty())
        {
            for(const std::string& script : m_evaluateScriptRequests)
				m_ultraLightImplementation->evaluateScript(script);

            m_evaluateScriptRequests.clear();
        }
        m_evaluateScriptRequestsMutex.unlock();

        m_ultraLightImplementation->waitForCopyFence();

        m_ultraLightImplementation->update(m_inputHandler);
        m_ultraLightImplementation->render();
        
        m_needUpdate = false;
    }
}

Wolf::UltraLight::UltraLightImplementation::UltraLightImplementation(uint32_t width, uint32_t height, const std::string& absoluteURL, std::string filePath, const ResourceNonOwner<InputHandler>& inputHandler)
	: m_filePath(std::move(filePath))
{
    m_lastUpdated = std::filesystem::last_write_time(m_filePath);
    m_viewListener.reset(new UltralightViewListener(inputHandler));

    Config config;
    ViewConfig viewConfig;
    viewConfig.initial_device_scale = 1.0;
    viewConfig.is_accelerated = false;
    viewConfig.is_transparent = true;

    Platform::instance().set_surface_factory(&m_surfaceFactory);
    Platform::instance().set_config(config);
    Platform::instance().set_font_loader(GetPlatformFontLoader());
    Platform::instance().set_file_system(GetPlatformFileSystem("."));
    Platform::instance().set_logger(this);
    m_renderer = Renderer::Create();
    m_view = m_renderer->CreateView(width, height, viewConfig, nullptr);
    m_view->set_load_listener(this);
    m_view->set_view_listener(m_viewListener.get());
    //m_view->LoadHTML(htmlString);
    m_view->LoadURL(absoluteURL.c_str());
    m_view->Focus();

    while (!m_done)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        m_renderer->Update();
        m_renderer->Render();
    }

    m_copyImageCommandBuffer.reset(CommandBuffer::createCommandBuffer(QueueType::TRANSFER, false));

    createOutputAndRecordCopyCommandBuffer(width, height);

    m_copyImageSemaphore.reset(Semaphore::createSemaphore(VK_PIPELINE_STAGE_TRANSFER_BIT));
    m_copyImageFence.reset(Fence::createFence(false));
}

void Wolf::UltraLight::UltraLightImplementation::OnFinishLoading(View* caller, uint64_t frame_id, bool is_main_frame, const String& url)
{
    if (is_main_frame)
    {
        m_done = true;
    }
}

void Wolf::UltraLight::UltraLightImplementation::LogMessage(LogLevel log_level, const String& message)
{
    switch (log_level)
    {
    case LogLevel::Error:
        Debug::sendError(String(message).utf8().data());
        break;
    case LogLevel::Warning:
        Debug::sendWarning(String(message).utf8().data());
        break;
    case LogLevel::Info:
        Debug::sendInfo(String(message).utf8().data());
        break;
    default:
        __debugbreak();
        break;
    }
}

void Wolf::UltraLight::UltraLightImplementation::OnDOMReady(View* caller, uint64_t frame_id, bool is_main_frame, const String& url)
{
    RefPtr<JSContext> context = caller->LockJSContext();
    SetJSContext(context->ctx());
}

Wolf::Image* Wolf::UltraLight::UltraLightImplementation::getImage() const
{
    return m_userInterfaceImage.get();
}

void Wolf::UltraLight::UltraLightImplementation::getJSObject(JSObject& outObject)
{
    outObject = JSGlobalObject();
}

void Wolf::UltraLight::UltraLightImplementation::evaluateScript(const std::string& script) const
{
    m_view->EvaluateScript(script.c_str());
}

void Wolf::UltraLight::UltraLightImplementation::waitForCopyFence() const
{
    m_copyImageFence->waitForFence();
    m_copyImageFence->resetFence();
}

bool Wolf::UltraLight::UltraLightImplementation::reloadIfModified()
{
    if (const std::filesystem::file_time_type time = std::filesystem::last_write_time(m_filePath); m_lastUpdated != time)
    {
        m_done = false;
        m_view->LoadURL(m_view->url());
        m_lastUpdated = time;

        while (!m_done)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            m_renderer->Update();
            m_renderer->Render();
        }

        return true;
    }

    return false;
}

void Wolf::UltraLight::UltraLightImplementation::update(const ResourceNonOwner<InputHandler>& inputHandler) const
{
    float xscale, yscale;
    glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &xscale, &yscale);

    inputHandler->lockCache(this);

    MouseEvent mouseEvent;

    float currentMousePosX, currentMousePosY;
    inputHandler->getMousePosition(currentMousePosX, currentMousePosY);
    mouseEvent.x = static_cast<int>(currentMousePosX);
    mouseEvent.y = static_cast<int>(currentMousePosY);

    mouseEvent.type = MouseEvent::kType_MouseMoved;
    mouseEvent.button = MouseEvent::kButton_Left;

    m_view->FireMouseEvent(mouseEvent);
    
    if (inputHandler->mouseButtonPressedThisFrame(GLFW_MOUSE_BUTTON_LEFT, this))
    {
        mouseEvent.type = MouseEvent::kType_MouseDown;
        mouseEvent.button = MouseEvent::kButton_Left;

        m_view->FireMouseEvent(mouseEvent);
    }
    else if (inputHandler->mouseButtonReleasedThisFrame(GLFW_MOUSE_BUTTON_LEFT, this))
    {
        mouseEvent.type = MouseEvent::kType_MouseUp;
        mouseEvent.button = MouseEvent::kButton_Left;

        m_view->FireMouseEvent(mouseEvent);
    }

    if (inputHandler->mouseButtonPressedThisFrame(GLFW_MOUSE_BUTTON_RIGHT, this))
    {
        mouseEvent.type = MouseEvent::kType_MouseDown;
        mouseEvent.button = MouseEvent::kButton_Right;

        m_view->FireMouseEvent(mouseEvent);
    }
    else if (inputHandler->mouseButtonReleasedThisFrame(GLFW_MOUSE_BUTTON_RIGHT, this))
    {
        mouseEvent.type = MouseEvent::kType_MouseUp;
        mouseEvent.button = MouseEvent::kButton_Right;

        m_view->FireMouseEvent(mouseEvent);
    }

    if (inputHandler->keyPressedThisFrame(GLFW_KEY_BACKSPACE, this))
    {
        KeyEvent keyEvent;
        keyEvent.type = KeyEvent::kType_KeyDown;
        keyEvent.virtual_key_code = 0x08; // code for backspace
        keyEvent.is_system_key = false;

        m_view->FireKeyEvent(keyEvent);
    }
    if (inputHandler->keyPressedThisFrame(GLFW_KEY_ENTER, this) || inputHandler->keyPressedThisFrame(GLFW_KEY_KP_ENTER, this))
    {
        KeyEvent keyEvent;
        keyEvent.type = KeyEvent::kType_Char;
        constexpr char character = 13;
        keyEvent.text = &character;
        keyEvent.is_system_key = false;

        m_view->FireKeyEvent(keyEvent);
    }

    float scrollX, scrollY;
    inputHandler->getScroll(scrollX, scrollY);

    if (scrollX != 0.0f || scrollY != 0.0f)
    {
        ScrollEvent scrollEvent{};
        scrollEvent.type = ScrollEvent::kType_ScrollByPixel;
        scrollEvent.delta_x = static_cast<int>(scrollX);
        scrollEvent.delta_y = static_cast<int>(scrollY);

        m_view->FireScrollEvent(scrollEvent);
    }

    const std::vector<int>& characterPressed = inputHandler->getCharactersPressedThisFrame(this);
    for(const int character : characterPressed)
    {
        KeyEvent keyEvent;
        keyEvent.type = KeyEvent::kType_Char;
        keyEvent.text = std::bit_cast<const char*>(&character);
        keyEvent.is_system_key = false;
        m_view->FireKeyEvent(keyEvent);
    }

    inputHandler->clearCache(this);
    inputHandler->unlockCache(this);
    
    m_renderer->Update();
}

void Wolf::UltraLight::UltraLightImplementation::render() const
{
    m_renderer->Render();
}

void Wolf::UltraLight::UltraLightImplementation::resize(uint32_t width, uint32_t height) const
{
    m_view->Resize(width, height);
    m_renderer->Update();
    m_renderer->Render();
}

void Wolf::UltraLight::UltraLightImplementation::submitCopyImageCommandBuffer() const
{
    const std::vector<const Semaphore*> waitSemaphores{ };
    const std::vector<const Semaphore*> signalSemaphores{ m_copyImageSemaphore.get() };
    m_copyImageCommandBuffer->submit(waitSemaphores, signalSemaphores, m_copyImageFence.get());
}

void Wolf::UltraLight::UltraLightImplementation::createOutputAndRecordCopyCommandBuffer(uint32_t width, uint32_t height)
{
    CreateImageInfo createImageInfo;
    createImageInfo.extent = { width, height, 1 };
    createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    createImageInfo.format = Format::R8G8B8A8_UNORM;
    createImageInfo.mipLevelCount = 1;
    createImageInfo.usage = ImageUsageFlagBits::SAMPLED | ImageUsageFlagBits::TRANSFER_DST;
    createImageInfo.imageTiling = VK_IMAGE_TILING_LINEAR;
    createImageInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    m_userInterfaceImage.reset(Image::createImage(createImageInfo));

    m_copyImageCommandBuffer->beginCommandBuffer();

    VkImageCopy copyRegion{};

    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.srcOffset = { 0, 0, 0 };

    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.baseArrayLayer = 0;
    copyRegion.dstSubresource.mipLevel = 0;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.dstOffset = { 0, 0, 0 };

    copyRegion.extent = { width, height, 1 };

    const UltraLightSurface* surface = dynamic_cast<UltraLightSurface*>(m_view->surface());
    m_userInterfaceImage->transitionImageLayout(*m_copyImageCommandBuffer, { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1 });
    m_userInterfaceImage->recordCopyGPUImage(surface->getImage(), copyRegion, *m_copyImageCommandBuffer);
    m_userInterfaceImage->transitionImageLayout(*m_copyImageCommandBuffer, { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 1 });

    m_copyImageCommandBuffer->endCommandBuffer();
}
