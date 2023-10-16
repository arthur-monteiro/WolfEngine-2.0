#pragma once

#ifndef __ANDROID__

#include "Window.h"

namespace Wolf
{
	class InputHandler
	{
	public:
		InputHandler(GLFWwindow* window);
		void moveToNextFrame();

		bool keyPressedThisFrame(int key) const;
		bool keyMaintained(int key) const;
		bool keyReleasedThisFrame(int key) const;

		const std::vector<int>& getCharactersPressedThisFrame() const { return m_charCache.inputPressedThisFrame; }

		bool mouseButtonPressedThisFrame(int button) const;
		bool mouseButtonReleasedThisFrame(int button) const;

		void inputHandlerKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		void inputHandlerCharCallback(GLFWwindow* window, unsigned int codepoint);
		void inputHandlerMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

		void getMousePosition(float& outX, float& outY) const;

	private:
		struct InputCache
		{
			std::vector<int> inputPressedForNextFrame;
			std::vector<int> inputReleasedForNextFrame;

			std::vector<int> inputPressedThisFrame;
			std::vector<int> inputReleasedThisFrame;
			std::vector<int> inputMaintained;
		};

		InputCache m_keysCache;
		InputCache m_charCache;
		InputCache m_mouseButtonsCache;

		float m_mousePosX, m_mousePosY;

		GLFWwindow* m_window;
	};
}

#endif
