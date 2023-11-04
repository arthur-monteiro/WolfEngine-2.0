#pragma once

#include <mutex>

#ifndef __ANDROID__

#include <map>

#include "Window.h"

namespace Wolf
{
	class InputHandler
	{
	public:
		InputHandler(GLFWwindow* window);
		void moveToNextFrame();

		void createCache(const void* instancePtr);
		void lockCache(const void* instancePtr);
		void pushDataToCache(const void* instancePtr);
		void clearCache(const void* instancePtr);
		void unlockCache(const void* instancePtr);

		bool keyPressedThisFrame(int key, const void* instancePtr = nullptr) const;
		bool keyMaintained(int key) const;
		bool keyReleasedThisFrame(int key, const void* instancePtr = nullptr) const;

		const std::vector<int>& getCharactersPressedThisFrame(const void* instancePtr = nullptr) const;

		bool mouseButtonPressedThisFrame(int button, const void* instancePtr = nullptr) const;
		bool mouseButtonReleasedThisFrame(int button, const void* instancePtr = nullptr) const;

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

			void clear()
			{
				inputPressedForNextFrame.clear();
				inputReleasedForNextFrame.clear();

				inputPressedThisFrame.clear();
				inputReleasedThisFrame.clear();
				inputMaintained.clear();
			}

			void concatenate(const InputCache& other)
			{
				auto concatenateIntVectors = [](std::vector<int>& output, const std::vector<int>& input)
				{
					if (input.empty())
						return;

					const uint32_t outputSizeBefore = static_cast<uint32_t>(output.size());
					const uint32_t newSize = outputSizeBefore + static_cast<uint32_t>(input.size());
					output.resize(newSize);
					memcpy(output.data() + outputSizeBefore, input.data(), input.size() * sizeof(int));
				};

				concatenateIntVectors(inputPressedThisFrame, other.inputPressedThisFrame);
				concatenateIntVectors(inputReleasedThisFrame, other.inputReleasedThisFrame);
				concatenateIntVectors(inputMaintained, other.inputMaintained);
			}
		};

		struct AllInputData
		{
			InputCache m_keysCache;
			InputCache m_charCache;
			InputCache m_mouseButtonsCache;

			float m_mousePosX, m_mousePosY;

			void clear()
			{
				m_keysCache.clear();
				m_charCache.clear();
				m_mouseButtonsCache.clear();
			}

			void concatenate(const AllInputData& other)
			{
				m_keysCache.concatenate(other.m_keysCache);
				m_charCache.concatenate(other.m_charCache);
				m_mouseButtonsCache.concatenate(other.m_mouseButtonsCache);
			}
		} m_data;

		std::map<const void*, std::pair<AllInputData, std::mutex>> m_dataCache;

		GLFWwindow* m_window;
	};
}

#endif
