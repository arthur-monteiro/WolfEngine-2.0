#pragma once

#ifndef __ANDROID__

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
        UltraLight(uint32_t width, uint32_t height, const std::string& absoluteURL);
        UltraLight(const UltraLight&) = delete;

        virtual void OnFinishLoading(ultralight::View* caller, uint64_t frame_id, bool is_main_frame, const ultralight::String& url) override;
        virtual void LogMessage(ultralight::LogLevel log_level, const ultralight::String16& message) override;
        virtual void OnDOMReady(ultralight::View* caller, uint64_t frame_id, bool is_main_frame, const ultralight::String& url) override;

        Image* getImage() const;
        static void getJSObject(ultralight::JSObject& outObject);
        void evaluateScript(const std::string& script) const;

        void update(GLFWwindow* window) const;
        void render() const;
        void resize(uint32_t width, uint32_t height) const;

    private:
        bool m_done = false;
        UltraLightSurfaceFactory m_surfaceFactory;

        ultralight::RefPtr<ultralight::Renderer> m_renderer;
        ultralight::RefPtr<ultralight::View> m_view;
    };
}

#endif
