#pragma once

#ifndef __ANDROID__

#include <array>
#include <functional>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace Wolf
{
	class Window
	{
	public:
		Window(const std::string& appName, uint32_t width, uint32_t height, bool borderless, void* systemManagerInstance, std::function<void(void*, int, int)> resizeCallback);

		static void pollEvents();
		bool windowShouldClose() const;
		bool windowVisible() const;
		void close() { m_closeRequested = true;}
		void minify();
		void getWindowPos(uint32_t& outX, uint32_t& outY) const;
		void setWindowPos(uint32_t posX, uint32_t posY) const;

		enum class CursorType { POINTER, HAND, IBEAM, HRESIZE, VRESIZE, Count };
		void setCursorType(CursorType cursorType) const;

		[[nodiscard]] GLFWwindow* getWindow() const { return m_window; }

	private:
		static void onWindowResized(GLFWwindow* window, int width, int height)
		{
			if (width == 0 || height == 0) return;

			const Window* windowManager = static_cast<Window*>(glfwGetWindowUserPointer(window));
			windowManager->callResizeCallback(width, height);
		}
		inline void callResizeCallback(int width, int height) const { m_resizeCallback(m_systemManagerInstance, width, height); }

		GLFWwindow* m_window;
		bool m_closeRequested = false;

		std::array<GLFWcursor*, 5> m_cursors;
		static_assert(std::tuple_size<decltype(m_cursors)>{} == static_cast<uint32_t>(CursorType::Count));

		void* m_systemManagerInstance = nullptr;
		std::function<void(void*, int, int)> m_resizeCallback;
	};
}

#endif