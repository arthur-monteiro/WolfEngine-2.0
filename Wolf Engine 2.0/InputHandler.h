#pragma once
#include "Debug.h"

#ifndef __ANDROID__

#include <map>
#include <mutex>

#include "ResourceNonOwner.h"
#include "Window.h"

namespace Wolf
{
	class InputHandler
	{
	public:
		InputHandler(const ResourceNonOwner<const Window>& window);
		~InputHandler();
		void moveToNextFrame();

		// Cache management
		void createCache(const void* instancePtr);
		void lockCache(const void* instancePtr);
		void pushDataToCache(const void* instancePtr);
		void clearCache(const void* instancePtr);
		void unlockCache(const void* instancePtr);

		// Get infos
		bool keyPressedThisFrame(int key, const void* instancePtr = nullptr) const;
		bool keyMaintained(int key) const;
		bool keyPressedThisFrameOrMaintained(int key, const void* instancePtr = nullptr) const;
		bool keyReleasedThisFrame(int key, const void* instancePtr = nullptr) const;

		const std::vector<int>& getCharactersPressedThisFrame(const void* instancePtr = nullptr) const;

		bool mouseButtonPressedThisFrame(int button, const void* instancePtr = nullptr) const;
		bool mouseButtonReleasedThisFrame(int button, const void* instancePtr = nullptr) const;

		void getMousePosition(float& outX, float& outY) const;

		void getScroll(float& outX, float& outY) const;

		// Callbacks
		void inputHandlerKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		void inputHandlerCharCallback(GLFWwindow* window, unsigned int codepoint);
		void inputHandlerMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
		void inputHandlerScrollCallback(GLFWwindow* window, double offsetX, double offsetY);

		// Actions
		void setCursorType(Window::CursorType cursorType) const;

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

		struct ScrollCache
		{
			struct ScrollEvent
			{
				float offsetX;
				float offsetY;
			};
			ScrollEvent scrollEventsForNextFrame;
			ScrollEvent scrollEventsThisFrame;

			void clear()
			{
				scrollEventsForNextFrame = { 0.0f, 0.0f };
				scrollEventsThisFrame = { 0.0f, 0.0f };
			}

			void concatenate(const ScrollCache& other)
			{
				scrollEventsThisFrame.offsetX += other.scrollEventsThisFrame.offsetX;
				scrollEventsThisFrame.offsetY += other.scrollEventsThisFrame.offsetY;
			}
		};

		struct AllInputData
		{
			InputCache m_keysCache;
			InputCache m_charCache;
			InputCache m_mouseButtonsCache;
			ScrollCache m_scrollCache;

			float m_mousePosX, m_mousePosY;

			void clear()
			{
				m_keysCache.clear();
				m_charCache.clear();
				m_mouseButtonsCache.clear();
				m_scrollCache.clear();
			}

			void concatenate(const AllInputData& other)
			{
				m_keysCache.concatenate(other.m_keysCache);
				m_charCache.concatenate(other.m_charCache);
				m_mouseButtonsCache.concatenate(other.m_mouseButtonsCache);
				m_scrollCache.concatenate(other.m_scrollCache);
			}
		} m_data;

		std::map<const void*, std::pair<AllInputData, std::mutex>> m_dataCache;

		ResourceNonOwner<const Window> m_window;
	};
}

#endif
