#include "InputHandler.h"

#include <glm/detail/func_common.hpp>

#include "Debug.h"

#ifndef __ANDROID__

inline static Wolf::InputHandler* inputHandlerInstance;

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

Wolf::InputHandler::~InputHandler()
{
	inputHandlerInstance = nullptr;

	glfwSetKeyCallback(m_window->getWindow(), nullptr);
	glfwSetCharCallback(m_window->getWindow(), nullptr);
	glfwSetMouseButtonCallback(m_window->getWindow(), nullptr);
}

void Wolf::InputHandler::moveToNextFrame()
{
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

void Wolf::InputHandler::getJoystickSpeedForGamepad(uint8_t gamepadIdx, uint8_t joystickIdx, float& outX, float& outY, const void* instancePtr)
{
	GamepadCache* gamepadCache = nullptr;

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

#endif
