#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include <WolfEngine.h>

#include "UniquePass.h"

struct VulkanEngine
{
    struct android_app *app;
    Wolf::WolfEngine* app_backend;
    bool canRender = false;
};

Wolf::ResourceUniqueOwner<UniquePass> pass;

void debugCallback(Wolf::Debug::Severity severity, Wolf::Debug::Type type, const std::string& message)
{
    if (severity == Wolf::Debug::Severity::VERBOSE)
        return;

    switch (severity)
    {
        case Wolf::Debug::Severity::ERROR:
            std::cout << "Error : ";
            break;
        case Wolf::Debug::Severity::WARNING:
            std::cout << "Warning : ";
            break;
        case Wolf::Debug::Severity::INFO:
            std::cout << "Info : ";
            break;
    }

    std::cout << message << "\n";
}

/**
 * Called by the Android runtime whenever events happen so the
 * app can react to it.
 */
static void HandleCmd(struct android_app *app, int32_t cmd) {
  auto *engine = (VulkanEngine *)app->userData;
  switch (cmd) {
    case APP_CMD_START:
    case APP_CMD_INIT_WINDOW:
        if (engine->app->window != nullptr)
        {
            Wolf::WolfInstanceCreateInfo wolfInstanceCreateInfo;
            wolfInstanceCreateInfo.m_configFilename = "";
            wolfInstanceCreateInfo.m_debugCallback = debugCallback;
            wolfInstanceCreateInfo.m_androidWindow = engine->app->window;
            wolfInstanceCreateInfo.m_assetManager = app->activity->assetManager;

            Wolf::WolfEngine* vulkanBackend = new Wolf::WolfEngine(wolfInstanceCreateInfo);

            pass.reset(new UniquePass);
            vulkanBackend->initializePass(pass.createNonOwnerResource<Wolf::CommandRecordBase>());

            engine->app_backend = vulkanBackend;
            engine->canRender = true;
        }
      break;
    case APP_CMD_TERM_WINDOW:
      // The window is being hidden or closed, clean it up.
      engine->canRender = false;
      break;
    case APP_CMD_DESTROY:
      // The window is being hidden or closed, clean it up.
      delete engine->app_backend;
    default:
      break;
  }
}

/*
 * Key events filter to GameActivity's android_native_app_glue. This sample does
 * not use/process any input events, return false for all input events so system
 * can still process them.
 */
extern "C" bool VulkanKeyEventFilter(const GameActivityKeyEvent *event) {
  return false;
}
extern "C" bool VulkanMotionEventFilter(const GameActivityMotionEvent *event) {
  return false;
}

/*
 * Process user touch and key events. GameActivity double buffers those events,
 * applications can process at any time. All of the buffered events have been
 * reported "handled" to OS. For details, refer to:
 * d.android.com/games/agdk/game-activity/get-started#handle-events
 */
static void HandleInputEvents(struct android_app *app) {
  auto inputBuf = android_app_swap_input_buffers(app);
  if (inputBuf == nullptr) {
    return;
  }

  // For the minimum, apps need to process the exit event (for example,
  // listening to AKEYCODE_BACK). This sample has done that in the Kotlin side
  // and not processing other input events, we just reset the event counter
  // inside the android_input_buffer to keep app glue code in a working state.
  android_app_clear_motion_events(inputBuf);
  android_app_clear_motion_events(inputBuf);
}

/*
 * Entry point required by the Android Glue library.
 * This can also be achieved more verbosely by manually declaring JNI functions
 * and calling them from the Android application layer.
 */
void android_main(struct android_app *state) {

    VulkanEngine engine{};

    engine.app = state;
    engine.app_backend = nullptr;
    state->userData = &engine;
    state->onAppCmd = HandleCmd;

    android_app_set_key_event_filter(state, VulkanKeyEventFilter);
    android_app_set_motion_event_filter(state, VulkanMotionEventFilter);

    while (true) {
        int ident;
        int events;
        android_poll_source *source;
        while ((ident = ALooper_pollAll(engine.canRender ? 0 : -1, nullptr, &events,
                                        (void **)&source)) >= 0) {
            if (source != nullptr) {
                source->process(state, source);
            }
        }

        HandleInputEvents(state);

        if(engine.app_backend)
        {
            std::vector<Wolf::ResourceNonOwner<Wolf::CommandRecordBase>> passes;
            passes.push_back(pass.createNonOwnerResource<Wolf::CommandRecordBase>());

            engine.app_backend->updateBeforeFrame();
            uint32_t swapChainImageIdx = engine.app_backend->acquireNextSwapChainImage();
            engine.app_backend->frame(passes, pass->getSemaphore(swapChainImageIdx), swapChainImageIdx);
        }
    }
}