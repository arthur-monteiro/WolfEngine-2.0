#include "UltraLight.h"

#include <chrono>
#include <thread>

#include "Debug.h"
#include "InputHandler.h"

using namespace ultralight;

Wolf::UltraLight::UltraLight(uint32_t width, uint32_t height, const std::string& absoluteURL, std::string filePath) : m_filePath(std::move(filePath))
{
    m_lastUpdated = std::filesystem::last_write_time(m_filePath);

    Config config;
    config.device_scale = 1.0;
    config.font_family_standard = "Arial";
    config.resource_path = "./resources/";
    config.use_gpu_renderer = false;

    Platform::instance().set_surface_factory(&m_surfaceFactory);
    Platform::instance().set_config(config);
    Platform::instance().set_font_loader(GetPlatformFontLoader());
    Platform::instance().set_file_system(GetPlatformFileSystem("."));
    Platform::instance().set_logger(this);
    m_renderer = Renderer::Create();
    m_view = m_renderer->CreateView(width, height, true, nullptr);
    m_view->set_load_listener(this);
    //m_view->LoadHTML(htmlString);
    m_view->LoadURL(absoluteURL.c_str());

    while (!m_done)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        m_renderer->Update();
        m_renderer->Render();
    }
}

void Wolf::UltraLight::OnFinishLoading(View* caller, uint64_t frame_id, bool is_main_frame, const String& url)
{
    if (is_main_frame)
    {
        m_done = true;
    }
}

void Wolf::UltraLight::LogMessage(LogLevel log_level, const String16& message)
{
    switch (log_level)
    {
    case kLogLevel_Error:
        Debug::sendError(String(message).utf8().data());
        break;
    case kLogLevel_Warning:
        Debug::sendWarning(String(message).utf8().data());
        break;
    case kLogLevel_Info:
        Debug::sendInfo(String(message).utf8().data());
        break;
    default:
        __debugbreak();
        break;
    }
}

void Wolf::UltraLight::OnDOMReady(View* caller, uint64_t frame_id, bool is_main_frame, const String& url)
{
    Ref<JSContext> context = caller->LockJSContext();
    SetJSContext(context.get());
}

Wolf::Image* Wolf::UltraLight::getImage() const
{
	const UltraLightSurface* surface = dynamic_cast<UltraLightSurface*>(m_view->surface());
    return surface->getImage();
}

void Wolf::UltraLight::getJSObject(JSObject& outObject)
{
    outObject = JSGlobalObject();
}

void Wolf::UltraLight::evaluateScript(const std::string& script) const
{
    m_view->EvaluateScript(script.c_str());
}

bool Wolf::UltraLight::reloadIfModified()
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

void Wolf::UltraLight::update(InputHandler* inputHandler) const
{
    float xscale, yscale;
    glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &xscale, &yscale);

    MouseEvent mouseEvent;

    float currentMousePosX, currentMousePosY;
    inputHandler->getMousePosition(currentMousePosX, currentMousePosY);
    mouseEvent.x = static_cast<int>(currentMousePosX);
    mouseEvent.y = static_cast<int>(currentMousePosY);

    mouseEvent.type = MouseEvent::kType_MouseMoved;
    mouseEvent.button = MouseEvent::kButton_Left;

    m_view->FireMouseEvent(mouseEvent);

    
    if (inputHandler->mouseButtonPressedThisFrame(GLFW_MOUSE_BUTTON_LEFT))
    {
        mouseEvent.type = MouseEvent::kType_MouseDown;
        mouseEvent.button = MouseEvent::kButton_Left;

        m_view->FireMouseEvent(mouseEvent);
    }
    else if (inputHandler->mouseButtonReleasedThisFrame(GLFW_MOUSE_BUTTON_LEFT))
    {
        mouseEvent.type = MouseEvent::kType_MouseUp;
        mouseEvent.button = MouseEvent::kButton_Left;

        m_view->FireMouseEvent(mouseEvent);
    }

    if (inputHandler->keyPressedThisFrame(GLFW_KEY_BACKSPACE))
    {
        KeyEvent keyEvent;
        keyEvent.type = KeyEvent::kType_KeyDown;
        keyEvent.virtual_key_code = 0x08; // code for backspace
        keyEvent.is_system_key = false;
        m_view->FireKeyEvent(keyEvent);
    }
    if (inputHandler->keyPressedThisFrame(GLFW_KEY_ENTER) || inputHandler->keyPressedThisFrame(GLFW_KEY_KP_ENTER))
    {
        KeyEvent keyEvent;
        keyEvent.type = KeyEvent::kType_KeyDown;
        keyEvent.virtual_key_code = 0x0D; // code for return
        keyEvent.is_system_key = false;
        m_view->FireKeyEvent(keyEvent);
    }    

    const std::vector<int>& characterPressed = inputHandler->getCharactersPressedThisFrame();
    for(const int character : characterPressed)
    {
        KeyEvent keyEvent;
        keyEvent.type = KeyEvent::kType_Char;
        keyEvent.text = reinterpret_cast<const char*>(&character);
        keyEvent.is_system_key = false;
        m_view->FireKeyEvent(keyEvent);
    }

    m_renderer->Update();
}

void Wolf::UltraLight::render() const
{
    m_renderer->Render();
}

void Wolf::UltraLight::resize(uint32_t width, uint32_t height) const
{
    m_view->Resize(width, height);
    m_renderer->Update();
    m_renderer->Render();
}
