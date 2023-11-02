#include "Window.h"

#include <utility>

Wolf::Window::Window(const std::string& appName, uint32_t width, uint32_t height, void* systemManagerInstance, std::function<void(void*, int, int)> resizeCallback)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	m_window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), appName.c_str(), nullptr, nullptr);

	glfwSetWindowUserPointer(m_window, this);
	glfwSetWindowSizeCallback(m_window, onWindowResized);
	//glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	m_systemManagerInstance = systemManagerInstance;
	m_resizeCallback = std::move(resizeCallback);
}

void Wolf::Window::pollEvents()
{
	glfwPollEvents();
}

bool Wolf::Window::windowShouldClose() const
{
	return glfwWindowShouldClose(m_window);
}

bool Wolf::Window::windowVisible() const
{
	return glfwGetWindowAttrib(m_window, GLFW_VISIBLE);
}
