#include "InputHandler.h"

#include <glm/detail/func_common.hpp>

#include <Debug.h>

#include "ProfilerCommon.h"

inline static Wolf::InputHandler* inputHandlerInstance;

#ifndef __ANDROID__
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	inputHandlerInstance->inputHandlerKeyCallback(window, key, scancode, action, mods);
}

void charCallback(GLFWwindow* window, unsigned codepoint)
{
	inputHandlerInstance->inputHandlerCharCallback(window, codepoint);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	inputHandlerInstance->inputHandlerMouseButtonCallback(window, button, action, mods);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	inputHandlerInstance->inputHandlerScrollCallback(window, xoffset, yoffset);
}

void joystickCallback(int jid, int event)
{
	inputHandlerInstance->inputHandlerJoystickCallback(jid, event);
}
#endif

#ifndef __ANDROID__
Wolf::InputHandler::InputHandler(const ResourceNonOwner<const Window>& window) : m_window(window)
{
	inputHandlerInstance = this;

	glfwSetKeyCallback(window->getWindow(), keyCallback);
	glfwSetCharCallback(window->getWindow(), charCallback);
	glfwSetMouseButtonCallback(window->getWindow(), mouseButtonCallback);
	glfwSetScrollCallback(window->getWindow(), scrollCallback);

	for (uint32_t gamepadIdx = 0; gamepadIdx < MAX_GAMEPAD_COUNT; ++gamepadIdx)
	{
		if (glfwJoystickIsGamepad(static_cast<int>(gamepadIdx)))
			m_data.m_gamepadCaches[gamepadIdx].isActiveNextFrame = true;
	}
	glfwSetJoystickCallback(joystickCallback);
}
#else
Wolf::InputHandler::InputHandler(struct android_app* androidApp)
: m_androidApp(androidApp), m_internalAndroidJoystickData { InternalAndroidJoystickData(glm::vec2(0.0f, 0.25f), glm::vec2(0.5f, 1.0f)), InternalAndroidJoystickData(glm::vec2(0.5f, 0.25f),  glm::vec2(1.0f, 1.0f)) }
{
    m_data.m_gamepadCaches[0].isActive = true; // activate the first gamepad

    android_app_set_motion_event_filter(m_androidApp, [](const GameActivityMotionEvent* event)
    {
        int32_t source = event->source;

        if ((source & AINPUT_SOURCE_TOUCHSCREEN) || (source & AINPUT_SOURCE_JOYSTICK))
        {
            return true;
        }
        return false;
    });
}
#endif

Wolf::InputHandler::~InputHandler()
{
    inputHandlerInstance = nullptr;

#ifndef __ANDROID__
    glfwSetKeyCallback(m_window->getWindow(), nullptr);
	glfwSetCharCallback(m_window->getWindow(), nullptr);
	glfwSetMouseButtonCallback(m_window->getWindow(), nullptr);
#endif
}

void Wolf::InputHandler::moveToNextFrame()
{
    PROFILE_FUNCTION

    m_frameIdx++;

    auto moveInputToNextFrame = [&](InputCache& inputCache)
    {
        // Release key released
        for (int key : inputCache.inputReleasedForNextFrame)
        {
            std::erase(inputCache.inputPressedThisFrame, key);
            std::erase(inputCache.inputMaintained, key);
        }
        inputCache.inputReleasedThisFrame.clear();
        inputCache.inputReleasedForNextFrame.swap(inputCache.inputReleasedThisFrame);

        // Move key pressed to maintained
        for (int key : inputCache.inputPressedThisFrame)
        {
            inputCache.inputMaintained.push_back(key);
        }

        inputCache.inputPressedThisFrame.swap(inputCache.inputPressedForNextFrame);
        inputCache.inputPressedForNextFrame.clear();
    };

    moveInputToNextFrame(m_data.m_keysCache);

    m_data.m_charCache.inputPressedThisFrame.clear();
    m_data.m_charCache.inputMaintained.clear();
    moveInputToNextFrame(m_data.m_charCache);

    moveInputToNextFrame(m_data.m_mouseButtonsCache);

#ifdef __ANDROID__
    handleAndroidInputs();

    const uint8_t gamepadIdx = 0;
    GamepadCache& gamepadCache = m_data.m_gamepadCaches[gamepadIdx];

    if (!gamepadCache.isActive)
    {
        Debug::sendMessageOnce("Gamepad 0 is not active", Debug::Severity::ERROR, this);
        return;
    }

    for (uint8_t joystickIdx = 0; joystickIdx < GAMEPAD_JOYSTICK_COUNT; ++joystickIdx)
    {
        const InternalAndroidJoystickData& internalAndroidJoystickData = m_internalAndroidJoystickData[joystickIdx];
        gamepadCache.joystickEvent[joystickIdx].offsetX = internalAndroidJoystickData.getOffsetX();
        gamepadCache.joystickEvent[joystickIdx].offsetY = internalAndroidJoystickData.getOffsetY();
    }
#else
    double currentMousePosX, currentMousePosY;
	glfwGetCursorPos(m_window->getWindow(), &currentMousePosX, &currentMousePosY);
	m_data.m_mousePosX = static_cast<float>(currentMousePosX);
	m_data.m_mousePosY = static_cast<float>(currentMousePosY);

	m_data.m_scrollCache.scrollEventsThisFrame = m_data.m_scrollCache.scrollEventsForNextFrame;
	m_data.m_scrollCache.scrollEventsForNextFrame = { 0.0f, 0.0f };

	// Gamepad
	{
		for (uint8_t gamepadIdx = 0; gamepadIdx < MAX_GAMEPAD_COUNT; ++gamepadIdx)
		{
			GamepadCache& gamepadCache = m_data.m_gamepadCaches[gamepadIdx];

			if (gamepadCache.isActiveNextFrame == false && gamepadCache.isActive)
				gamepadCache.isActive = false;
			else if(gamepadCache.isActiveNextFrame == true && !gamepadCache.isActive)
			{
				gamepadCache.isActive = true;
				gamepadCache.clear();
			}

			if (!gamepadCache.isActive)
				continue;

			GLFWgamepadstate state;
			if (glfwGetGamepadState(gamepadIdx, &state))
			{
				for (uint8_t joystickIdx = 0; joystickIdx < GAMEPAD_JOYSTICK_COUNT; ++joystickIdx)
				{
					gamepadCache.joystickEvent[joystickIdx].offsetX = state.axes[static_cast<size_t>(2 * joystickIdx)];
					gamepadCache.joystickEvent[joystickIdx].offsetY = state.axes[static_cast<size_t>(2 * joystickIdx + 1)];

					if (glm::abs(gamepadCache.joystickEvent[joystickIdx].offsetX) < GAMEPAD_JOYSTICK_DEAD_ZONE_SIZE && glm::abs(gamepadCache.joystickEvent[joystickIdx].offsetY) < GAMEPAD_JOYSTICK_DEAD_ZONE_SIZE)
					{
						gamepadCache.joystickEvent[joystickIdx].offsetX = 0.0f;
						gamepadCache.joystickEvent[joystickIdx].offsetY = 0.0f;
					}
				}

				for (uint8_t triggerIdx = 0; triggerIdx < GAMEPAD_TRIGGER_COUNT; ++triggerIdx)
				{
					gamepadCache.triggerEvents[triggerIdx] = state.axes[static_cast<size_t>(4 + triggerIdx)];
				}

				for (uint8_t buttonIdx = 0; buttonIdx < GAMEPAD_BUTTON_COUNT; ++buttonIdx)
				{
					gamepadCache.buttons[buttonIdx] = state.buttons[buttonIdx] == GLFW_PRESS ? GamepadCache::GamepadButtonState::PRESSED : GamepadCache::GamepadButtonState::RELEASED;
				}
			}
			else
			{
				Debug::sendError("Can't get state for gamepad " + std::to_string(gamepadIdx));
			}
		}
	}
#endif
}

void Wolf::InputHandler::lockCache(const void* instancePtr)
{
    m_dataCache[instancePtr].second.lock();
}

void Wolf::InputHandler::pushDataToCache(const void* instancePtr)
{
    if (m_dataCache.contains(instancePtr))
        m_dataCache[instancePtr].first.concatenate(m_data);
    else
        Debug::sendCriticalError("No cache found");
}

void Wolf::InputHandler::clearCache(const void* instancePtr)
{
    m_dataCache[instancePtr].first.clear();
}

void Wolf::InputHandler::unlockCache(const void* instancePtr)
{
    m_dataCache.at(instancePtr).second.unlock();
}

#ifndef __ANDROID__
bool Wolf::InputHandler::keyPressedThisFrame(int key, const void* instancePtr) const
{
	if (instancePtr)
		return std::ranges::find(m_dataCache.at(instancePtr).first.m_keysCache.inputPressedThisFrame, key) != m_dataCache.at(instancePtr).first.m_keysCache.inputPressedThisFrame.end();

	return std::ranges::find(m_data.m_keysCache.inputPressedThisFrame, key) != m_data.m_keysCache.inputPressedThisFrame.end();
}

bool Wolf::InputHandler::keyMaintained(int key) const
{
	return std::ranges::find(m_data.m_keysCache.inputMaintained, key) != m_data.m_keysCache.inputMaintained.end();
}

bool Wolf::InputHandler::keyPressedThisFrameOrMaintained(int key, const void* instancePtr) const
{
	return keyPressedThisFrame(key, instancePtr) || keyMaintained(key);
}

bool Wolf::InputHandler::keyReleasedThisFrame(int key, const void* instancePtr) const
{
	if (instancePtr)
		return std::ranges::find(m_dataCache.at(instancePtr).first.m_keysCache.inputReleasedThisFrame, key) != m_dataCache.at(instancePtr).first.m_keysCache.inputReleasedThisFrame.end();

	return std::ranges::find(m_data.m_keysCache.inputReleasedThisFrame, key) != m_data.m_keysCache.inputReleasedThisFrame.end();
}

const std::vector<int>& Wolf::InputHandler::getCharactersPressedThisFrame(const void* instancePtr) const
{
	if (instancePtr)
		return m_dataCache.at(instancePtr).first.m_charCache.inputPressedThisFrame;

	return m_data.m_charCache.inputPressedThisFrame;
}

bool Wolf::InputHandler::mouseButtonPressedThisFrame(int button, const void* instancePtr) const
{
	if (instancePtr)
		return std::ranges::find(m_dataCache.at(instancePtr).first.m_mouseButtonsCache.inputPressedThisFrame, button) != m_dataCache.at(instancePtr).first.m_mouseButtonsCache.inputPressedThisFrame.end();

	return std::ranges::find(m_data.m_mouseButtonsCache.inputPressedThisFrame, button) != m_data.m_mouseButtonsCache.inputPressedThisFrame.end();
}

bool Wolf::InputHandler::mouseButtonReleasedThisFrame(int button, const void* instancePtr) const
{
	if (instancePtr)
		return std::ranges::find(m_dataCache.at(instancePtr).first.m_mouseButtonsCache.inputReleasedThisFrame, button) != m_dataCache.at(instancePtr).first.m_mouseButtonsCache.inputReleasedThisFrame.end();

	return std::ranges::find(m_data.m_mouseButtonsCache.inputReleasedThisFrame, button) != m_data.m_mouseButtonsCache.inputReleasedThisFrame.end();
}

void Wolf::InputHandler::inputHandlerKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		m_data.m_keysCache.inputPressedForNextFrame.push_back(key);
	}
	else if (action == GLFW_RELEASE)
	{
		m_data.m_keysCache.inputReleasedForNextFrame.push_back(key);
	}
}

void Wolf::InputHandler::inputHandlerCharCallback(GLFWwindow* window, unsigned codepoint)
{
	m_data.m_charCache.inputPressedForNextFrame.push_back(codepoint);
}

void Wolf::InputHandler::inputHandlerMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		m_data.m_mouseButtonsCache.inputPressedForNextFrame.push_back(button);
	}
	else if (action == GLFW_RELEASE)
	{
		m_data.m_mouseButtonsCache.inputReleasedForNextFrame.push_back(button);
	}
}

void Wolf::InputHandler::inputHandlerScrollCallback(GLFWwindow* window, double offsetX, double offsetY)
{
	int windowWidth, windowHeight;
	glfwGetWindowSize(window, &windowWidth, &windowHeight);

	m_data.m_scrollCache.scrollEventsForNextFrame.offsetX += static_cast<float>(offsetX) * static_cast<float>(windowWidth);
	m_data.m_scrollCache.scrollEventsForNextFrame.offsetY += static_cast<float>(offsetY) * static_cast<float>(windowHeight);
}

void Wolf::InputHandler::inputHandlerJoystickCallback(int jid, int event)
{
	if (jid >= MAX_GAMEPAD_COUNT)
	{
		Debug::sendError("Too many gamepads");
		return;
	}

	if (event == GLFW_CONNECTED)
	{
		if (!glfwJoystickIsGamepad(jid))
		{
			Debug::sendError("Unrecognised gamepad");
			return;
		}

		Debug::sendInfo("Gamepad " + std::to_string(jid) + " connected");
		m_data.m_gamepadCaches[jid].isActiveNextFrame = true;
	}
	else if (event == GLFW_DISCONNECTED)
	{
		Debug::sendInfo("Gamepad " + std::to_string(jid) + " disconnected");
		m_data.m_gamepadCaches[jid].isActiveNextFrame = false;
	}
}

void Wolf::InputHandler::setCursorType(Window::CursorType cursorType) const
{
	m_window->setCursorType(cursorType);
}

void Wolf::InputHandler::getMousePosition(float& outX, float& outY) const
{
	outX = m_data.m_mousePosX;
	outY = m_data.m_mousePosY;
}

void Wolf::InputHandler::getScroll(float& outX, float& outY) const
{
	outX = m_data.m_scrollCache.scrollEventsThisFrame.offsetX;
	outY = m_data.m_scrollCache.scrollEventsThisFrame.offsetY;
}

#endif

void Wolf::InputHandler::getJoystickSpeedForGamepad(uint8_t gamepadIdx, uint8_t joystickIdx, float& outX, float& outY, const void* instancePtr) const
{
    const GamepadCache* gamepadCache = nullptr;

    if (instancePtr)
        gamepadCache = &m_dataCache.at(instancePtr).first.m_gamepadCaches[gamepadIdx];
    else
        gamepadCache = &m_data.m_gamepadCaches[gamepadIdx];

    if (gamepadCache->isActive)
    {
        outX = gamepadCache->joystickEvent[joystickIdx].offsetX;
        outY = gamepadCache->joystickEvent[joystickIdx].offsetY;
    }
    else
    {
        Debug::sendMessageOnce("Requesting joystick speed for an inactive controller", Debug::Severity::WARNING, this);
        outX = 0.0f;
        outY = 0.0f;
    }
}

float Wolf::InputHandler::getTriggerValueForGamepad(uint8_t gamepadIdx, uint8_t triggerIdx, const void* instancePtr)
{
    GamepadCache* gamepadCache = nullptr;

    if (instancePtr)
        gamepadCache = &m_dataCache.at(instancePtr).first.m_gamepadCaches[gamepadIdx];
    else
        gamepadCache = &m_data.m_gamepadCaches[gamepadIdx];

    if (gamepadCache->isActive)
    {
        return gamepadCache->triggerEvents[triggerIdx];
    }
    else
    {
        Debug::sendMessageOnce("Requesting trigger value for an inactive controller", Debug::Severity::WARNING, this);
        return 0.0f;
    }
}

bool Wolf::InputHandler::isGamepadButtonPressed(uint8_t gamepadIdx, uint8_t buttonIdx, const void* instancePtr)
{
    GamepadCache* gamepadCache = nullptr;

    if (instancePtr)
        gamepadCache = &m_dataCache.at(instancePtr).first.m_gamepadCaches[gamepadIdx];
    else
        gamepadCache = &m_data.m_gamepadCaches[gamepadIdx];

    if (gamepadCache->isActive)
    {
        return gamepadCache->buttons[buttonIdx] == GamepadCache::GamepadButtonState::PRESSED;
    }
    else
    {
        Debug::sendMessageOnce("Requesting button value for an inactive controller", Debug::Severity::WARNING, this);
        return false;
    }
}

#ifdef __ANDROID__
void Wolf::InputHandler::handleAndroidInputs()
{
    android_input_buffer* inputBuffer = android_app_swap_input_buffers(m_androidApp);
    if (inputBuffer && inputBuffer->motionEventsCount > 0)
    {
        for (uint64_t i = 0; i < inputBuffer->motionEventsCount; ++i)
        {
            GameActivityMotionEvent* event = &inputBuffer->motionEvents[i];

            int32_t action = event->action & AMOTION_EVENT_ACTION_MASK;
            int32_t actionIdx = (event->action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

            if (event->source & AINPUT_SOURCE_TOUCHSCREEN)
            {
                int32_t windowWidth = ANativeWindow_getWidth(m_androidApp->window);
                int32_t windowHeight = ANativeWindow_getHeight(m_androidApp->window);

                if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN)
                {
                    int32_t id = event->pointers[actionIdx].id;
                    glm::vec2 pos = glm::vec2(event->pointers[actionIdx].rawX / static_cast<float>(windowWidth), event->pointers[actionIdx].rawY / static_cast<float>(windowHeight));

                    for (InternalAndroidJoystickData& joystick : m_internalAndroidJoystickData)
                    {
                        joystick.tryActivate(id, pos);
                    }
                }
                else if (action == AMOTION_EVENT_ACTION_MOVE)
                {
                    if (actionIdx != 0)
                    {
                        Debug::sendError("Here actionIdx is supposed to be 0 but it's " + std::to_string(actionIdx));
                    }

                    for (uint32_t pointerIdx = 0; pointerIdx < event->pointerCount; ++pointerIdx)
                    {
                        int32_t id = event->pointers[pointerIdx].id;
                        glm::vec2 pos = glm::vec2(event->pointers[pointerIdx].rawX / static_cast<float>(windowWidth), event->pointers[pointerIdx].rawY / static_cast<float>(windowHeight));

                        for (InternalAndroidJoystickData& joystick : m_internalAndroidJoystickData)
                        {
                            joystick.tryMove(id, pos);
                        }
                    }
                }
                else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL || action == AMOTION_EVENT_ACTION_POINTER_UP)
                {
                    int32_t id = event->pointers[actionIdx].id;

                    for (InternalAndroidJoystickData& joystick : m_internalAndroidJoystickData)
                    {
                        joystick.tryDeactivate(id);
                    }
                }
            }

            if (event->source & AINPUT_SOURCE_JOYSTICK)
            {
                float axisX = GameActivityPointerAxes_getAxisValue(&event->pointers[0], AMOTION_EVENT_AXIS_X);
                float axisY = GameActivityPointerAxes_getAxisValue(&event->pointers[0], AMOTION_EVENT_AXIS_Y);

                m_data.m_gamepadCaches[0].isActive = true;
                m_data.m_gamepadCaches[0].joystickEvent[0].offsetX = axisX;
                m_data.m_gamepadCaches[0].joystickEvent[0].offsetY = axisY;
            }
        }
        android_app_clear_motion_events(inputBuffer);

        if (inputBuffer->keyEventsCount > 0)
        {
            android_app_clear_key_events(inputBuffer);
        }
    }

    for (InternalAndroidJoystickData& joystick : m_internalAndroidJoystickData)
    {
        joystick.touch(m_frameIdx);
    }
}

const glm::vec2& Wolf::InputHandler::getJoystickPosForVirtualGamepad(uint8_t joystickIdx) const
{
    return m_internalAndroidJoystickData[joystickIdx].getPosition();
}

const glm::vec2 &Wolf::InputHandler::getJoystickCenterForVirtualGamepad(uint8_t joystickIdx) const
{
    return m_internalAndroidJoystickData[joystickIdx].getCenter();
}

uint32_t Wolf::InputHandler::getLastActiveFrameIdxForVirtualGamepad(uint8_t joystickIdx) const
{
    return m_internalAndroidJoystickData[joystickIdx].getLastActiveFrameIndex();
}

Wolf::InputHandler::InternalAndroidJoystickData::InternalAndroidJoystickData(const glm::vec2 &minActivationPos, const glm::vec2 &maxActivationPos)
        : m_minActivationPos(minActivationPos), m_maxActivationPos(maxActivationPos)
{
}

bool Wolf::InputHandler::InternalAndroidJoystickData::tryActivate(int32_t id, const glm::vec2& normalizedPos)
{
    if (normalizedPos.x > m_minActivationPos.x && normalizedPos.x < m_maxActivationPos.x &&
        normalizedPos.y > m_minActivationPos.y && normalizedPos.y < m_maxActivationPos.y)
    {
        if (m_id != -1)
        {
            Debug::sendInfo("Joystick already activated");
            return false;
        }

        m_center = normalizedPos;
        m_position = normalizedPos;
        m_id = id;

        return true;
    }

    return false;
}

bool Wolf::InputHandler::InternalAndroidJoystickData::tryMove(int32_t id, const glm::vec2 &normalizedPos)
{
    if (m_id != id)
    {
        return false;
    }

    m_position = normalizedPos;
    m_direction = m_position - m_center;

    if (glm::length(m_direction) > 0.1f)
    {
        m_center = m_position - glm::normalize(m_direction) * 0.1f;
    }

    m_direction = glm::clamp(glm::vec2(-1.0f), glm::vec2(1.0f), m_direction / 0.1f);
    m_direction.y = -m_direction.y;

    return true;
}

bool Wolf::InputHandler::InternalAndroidJoystickData::tryDeactivate(int32_t id)
{
    if (m_id != id)
    {
        return false;
    }

    m_id = -1;
    return true;
}

void Wolf::InputHandler::InternalAndroidJoystickData::touch(uint32_t frameIdx)
{
    if (m_id != -1)
    {
        m_lastActiveFrameIndex = frameIdx;
    }
}

#endif