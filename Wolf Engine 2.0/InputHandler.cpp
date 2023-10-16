#include "InputHandler.h"

#ifndef __ANDROID__

static Wolf::InputHandler* inputHandlerInstance;

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

Wolf::InputHandler::InputHandler(GLFWwindow* window) : m_window(window)
{
	inputHandlerInstance = this;

	glfwSetKeyCallback(window, keyCallback);
	glfwSetCharCallback(window, charCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
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

	moveInputToNextFrame(m_keysCache);

	m_charCache.inputPressedThisFrame.clear();
	m_charCache.inputMaintained.clear();
	moveInputToNextFrame(m_charCache);

	moveInputToNextFrame(m_mouseButtonsCache);

	double currentMousePosX, currentMousePosY;
	glfwGetCursorPos(m_window, &currentMousePosX, &currentMousePosY);
	m_mousePosX = static_cast<float>(currentMousePosX);
	m_mousePosY = static_cast<float>(currentMousePosY);
}

bool Wolf::InputHandler::keyPressedThisFrame(int key) const
{
	return std::ranges::find(m_keysCache.inputPressedThisFrame, key) != m_keysCache.inputPressedThisFrame.end();
}

bool Wolf::InputHandler::keyMaintained(int key) const
{
	return std::ranges::find(m_keysCache.inputMaintained, key) != m_keysCache.inputMaintained.end();
}

bool Wolf::InputHandler::keyReleasedThisFrame(int key) const
{
	return std::ranges::find(m_keysCache.inputReleasedThisFrame, key) != m_keysCache.inputReleasedThisFrame.end();
}

bool Wolf::InputHandler::mouseButtonPressedThisFrame(int button) const
{
	return std::ranges::find(m_mouseButtonsCache.inputPressedThisFrame, button) != m_mouseButtonsCache.inputPressedThisFrame.end();
}

bool Wolf::InputHandler::mouseButtonReleasedThisFrame(int button) const
{
	return std::ranges::find(m_mouseButtonsCache.inputReleasedThisFrame, button) != m_mouseButtonsCache.inputReleasedThisFrame.end();
}

void Wolf::InputHandler::inputHandlerKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		m_keysCache.inputPressedForNextFrame.push_back(key);
	}
	else if (action == GLFW_RELEASE)
	{
		m_keysCache.inputReleasedForNextFrame.push_back(key);
	}
}

void Wolf::InputHandler::inputHandlerCharCallback(GLFWwindow* window, unsigned codepoint)
{
	m_charCache.inputPressedForNextFrame.push_back(codepoint);
}

void Wolf::InputHandler::inputHandlerMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		m_mouseButtonsCache.inputPressedForNextFrame.push_back(button);
	}
	else if (action == GLFW_RELEASE)
	{
		m_mouseButtonsCache.inputReleasedForNextFrame.push_back(button);
	}
}

void Wolf::InputHandler::getMousePosition(float& outX, float& outY) const
{
	outX = m_mousePosX;
	outY = m_mousePosY;
}

#endif