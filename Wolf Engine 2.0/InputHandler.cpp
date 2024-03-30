#include "InputHandler.h"

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

Wolf::InputHandler::InputHandler(const ResourceNonOwner<const Window>& window) : m_window(window)
{
	inputHandlerInstance = this;

	glfwSetKeyCallback(window->getWindow(), keyCallback);
	glfwSetCharCallback(window->getWindow(), charCallback);
	glfwSetMouseButtonCallback(window->getWindow(), mouseButtonCallback);
	glfwSetScrollCallback(window->getWindow(), scrollCallback);
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
}

void Wolf::InputHandler::createCache(const void* instancePtr)
{
	m_dataCache[instancePtr];
}

void Wolf::InputHandler::lockCache(const void* instancePtr)
{
	m_dataCache.at(instancePtr).second.lock();
}

void Wolf::InputHandler::pushDataToCache(const void* instancePtr)
{
	if (m_dataCache.contains(instancePtr))
		m_dataCache[instancePtr].first.concatenate(m_data);
	else
		Debug::sendError("No cache found");
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
