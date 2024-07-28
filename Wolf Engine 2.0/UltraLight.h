#pragma once

#ifndef __ANDROID__

#include <mutex>
#include <thread>

#include <AppCore/App.h>
#include <AppCore/JSHelpers.h>
#include <AppCore/Platform.h>
#include <filesystem>
#include <Ultralight/Ultralight.h>
#include <Ultralight/platform/Logger.h>

#include "CommandBuffer.h"
#include "Fence.h"
#include "ResourceNonOwner.h"
#include "UltraLightSurface.h"
#include "UltralightViewListener.h"

namespace Wolf
{
    class InputHandler;

    class UltraLight
    {
    public:
        UltraLight(const char* htmlURL, const std::function<void()>& bindCallbacks, const ResourceNonOwner<InputHandler>& inputHandler);
        ~UltraLight();

        void waitInitializationDone() const;

        void processFrameJobs();
        void requestScriptEvaluation(const std::string& script);

        void resize(uint32_t width, uint32_t height);

        static void getJSObject(ultralight::JSObject& outObject) { UltraLightImplementation::getJSObject(outObject); }
        const Semaphore* getImageCopySemaphore() const { return m_copySubmittedThisFrame ? m_ultraLightImplementation->getImageCopySemaphore() : nullptr; }
        Image* getImage() const { return m_ultraLightImplementation->getImage(); }

    private:
        void processImplementation(const char* htmlURL, const std::function<void()>& bindCallbacks);

        std::thread m_thread;
        std::mutex m_mutex;
        std::condition_variable m_updateCondition;
        std::function<void()> m_bindUltralightCallbacks;
        bool m_copySubmittedThisFrame = false;
        ResourceNonOwner<InputHandler> m_inputHandler;

        bool m_needUpdate = false;
        bool m_stopThreadRequested = false;
        std::mutex m_evaluateScriptRequestsMutex;
        std::vector<std::string> m_evaluateScriptRequests;
        VkExtent2D m_resizeRequest = { 0, 0 };

        class UltraLightImplementation : public ultralight::LoadListener, public ultralight::Logger
        {
        public:
            UltraLightImplementation(uint32_t width, uint32_t height, const std::string& absoluteURL,
                                     std::string filePath, const ResourceNonOwner<InputHandler>& inputHandler);
            UltraLightImplementation(const UltraLightImplementation&) = delete;

            virtual void OnFinishLoading(ultralight::View* caller, uint64_t frame_id, bool is_main_frame, const ultralight::String& url) override;
            virtual void LogMessage(ultralight::LogLevel log_level, const ultralight::String& message) override;
            virtual void OnDOMReady(ultralight::View* caller, uint64_t frame_id, bool is_main_frame, const ultralight::String& url) override;

            Image* getImage() const;
            static void getJSObject(ultralight::JSObject& outObject);
            void evaluateScript(const std::string& script) const;

            void waitForCopyFence() const;
            bool reloadIfModified();
            void update(const ResourceNonOwner<InputHandler>& inputHandler) const;
            void render() const;
            void resize(uint32_t width, uint32_t height) const;

            void createOutputAndRecordCopyCommandBuffer(uint32_t width, uint32_t height);

            void submitCopyImageCommandBuffer() const;
            const Semaphore* getImageCopySemaphore() const { return m_copyImageSemaphore.get(); }

        private:
            bool m_done = false;
            UltraLightSurfaceFactory m_surfaceFactory;

            ultralight::RefPtr<ultralight::Renderer> m_renderer;
            ultralight::RefPtr<ultralight::View> m_view;
            std::unique_ptr<UltralightViewListener> m_viewListener;

            std::string m_filePath;
            std::filesystem::file_time_type m_lastUpdated;

            // Copy to used UI image
            std::unique_ptr<Image> m_userInterfaceImage;
            std::unique_ptr<CommandBuffer> m_copyImageCommandBuffer;
            std::unique_ptr<Semaphore> m_copyImageSemaphore;
            std::unique_ptr<Fence> m_copyImageFence;
        };

        std::unique_ptr<UltraLightImplementation> m_ultraLightImplementation;
    };
}

#endif
