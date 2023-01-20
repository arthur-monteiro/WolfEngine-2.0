#include "UltraLight.h"

#include "Debug.h"

using namespace ultralight;

Wolf::UltraLight::UltraLight(uint32_t width, uint32_t height, const char* htmlString)
{
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
    m_view->LoadHTML(htmlString);

    while (!m_done)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        m_renderer->Update();
        m_renderer->Render();
    }
}

void Wolf::UltraLight::OnFinishLoading(ultralight::View* caller, uint64_t frame_id, bool is_main_frame, const ultralight::String& url)
{
    if (is_main_frame)
    {
        m_done = true;
    }
}

void Wolf::UltraLight::LogMessage(ultralight::LogLevel log_level, const ultralight::String16& message)
{
    switch (log_level)
    {
    case ultralight::kLogLevel_Error:
        Debug::sendError(ultralight::String(message).utf8().data());
        break;
    case ultralight::kLogLevel_Warning:
        Debug::sendWarning(ultralight::String(message).utf8().data());
        break;
    case ultralight::kLogLevel_Info:
        Debug::sendInfo(ultralight::String(message).utf8().data());
        break;
    default:
        break;
    }
}

void Wolf::UltraLight::OnDOMReady(ultralight::View* caller, uint64_t frame_id, bool is_main_frame, const ultralight::String& url)
{
    Ref<JSContext> context = caller->LockJSContext();
    SetJSContext(context.get());
}

Wolf::Image* Wolf::UltraLight::getImage()
{
    UltraLightSurface* surface = (UltraLightSurface*)m_view->surface();
    return surface->getImage();
}

void Wolf::UltraLight::getJSObject(JSObject& outObject)
{
    outObject = JSGlobalObject();
}

void Wolf::UltraLight::update(GLFWwindow* window)
{
    float xscale, yscale;
    glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &xscale, &yscale);

    MouseEvent mouseEvent;

    double currentMousePosX, currentMousePosY;
    glfwGetCursorPos(window, &currentMousePosX, &currentMousePosY);
    mouseEvent.x = currentMousePosX;
    mouseEvent.y = currentMousePosY;

    mouseEvent.type = MouseEvent::kType_MouseMoved;
    mouseEvent.button == MouseEvent::kButton_Left;

    m_view->FireMouseEvent(mouseEvent);

    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (state == GLFW_PRESS)
    {
        mouseEvent.type = MouseEvent::kType_MouseDown;
        mouseEvent.button == MouseEvent::kButton_Left;

        m_view->FireMouseEvent(mouseEvent);
    }
    else if (state == GLFW_RELEASE)
    {
        mouseEvent.type = MouseEvent::kType_MouseUp;
        mouseEvent.button == MouseEvent::kButton_Left;

        m_view->FireMouseEvent(mouseEvent);
    }

    m_renderer->Update();
}

void Wolf::UltraLight::render()
{
    m_renderer->Render();
}

void Wolf::UltraLight::resize(uint32_t width, uint32_t height)
{
    m_view->Resize(width, height);
    m_renderer->Update();
    m_renderer->Render();
}
