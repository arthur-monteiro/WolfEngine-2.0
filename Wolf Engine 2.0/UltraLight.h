#pragma once

#include <AppCore/App.h>
#include <AppCore/JSHelpers.h>
#include <AppCore/Platform.h>
#include <Ultralight/Ultralight.h>
#include <Ultralight/platform/Logger.h>

#include "UltraLightSurface.h"
#include "Window.h"

namespace Wolf
{
    class UltraLight : public ultralight::LoadListener, public ultralight::Logger
    {
    public:
        UltraLight(uint32_t width, uint32_t height, const char* htmlString);
        UltraLight(const UltraLight&) = delete;

        virtual void OnFinishLoading(ultralight::View* caller, uint64_t frame_id, bool is_main_frame, const ultralight::String& url) override;
        virtual void LogMessage(ultralight::LogLevel log_level, const ultralight::String16& message) override;
        virtual void OnDOMReady(ultralight::View* caller, uint64_t frame_id, bool is_main_frame, const ultralight::String& url) override;

        Image* getImage();
        void getJSObject(ultralight::JSObject& outObject);

        void update(GLFWwindow* window);
        void render();
        void resize(uint32_t width, uint32_t height);

    private:
        bool m_done = false;
        UltraLightSurfaceFactory m_surfaceFactory;

        ultralight::RefPtr<ultralight::Renderer> m_renderer;
        ultralight::RefPtr<ultralight::View> m_view;
    };
}