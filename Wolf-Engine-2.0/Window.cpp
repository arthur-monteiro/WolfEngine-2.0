#include "Window.h"

#ifndef __ANDROID__

#include <utility>

Wolf::Window::Window(const std::string& appName, uint32_t width, uint32_t height, bool borderless, void* systemManagerInstance, std::function<void(void*, int, int)> resizeCallback)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	if (borderless)
	{
		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	}

	m_window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), appName.c_str(), nullptr, nullptr);

	glfwSetWindowUserPointer(m_window, this);
	glfwSetWindowSizeCallback(m_window, onWindowResized);
	//glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	m_systemManagerInstance = systemManagerInstance;
	m_resizeCallback = std::move(resizeCallback);

	m_cursors[0] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	m_cursors[1] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
	m_cursors[2] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
	m_cursors[3] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
	m_cursors[4] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
}

void Wolf::Window::pollEvents()
{
	glfwPollEvents();
}

bool Wolf::Window::windowShouldClose() const
{
	return m_closeRequested || glfwWindowShouldClose(m_window);
}

bool Wolf::Window::windowVisible() const
{
	return glfwGetWindowAttrib(m_window, GLFW_VISIBLE);
}

void Wolf::Window::minify()
{
	glfwIconifyWindow(m_window);
}

void Wolf::Window::getWindowPos(uint32_t& outX, uint32_t& outY) const
{
	int winX, winY;
	glfwGetWindowPos(m_window, &winX, &winY);

	outX = winX;
	outY = winY;
}

void Wolf::Window::setWindowPos(uint32_t posX, uint32_t posY) const
{
	glfwSetWindowPos(m_window, static_cast<int>(posX), static_cast<int>(posY));
}

void Wolf::Window::setCursorType(CursorType cursorType) const
{
	glfwSetCursor(m_window, m_cursors[static_cast<uint32_t>(cursorType)]);
}

#endif