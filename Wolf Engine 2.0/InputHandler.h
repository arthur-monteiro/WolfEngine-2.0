#pragma once

#ifndef __ANDROID__

#include <map>
#include <mutex>

#include "Debug.h"
#include "ResourceNonOwner.h"
#include "Window.h"

namespace Wolf
{
	class InputHandler
	{
	public:
		static constexpr uint8_t MAX_GAMEPAD_COUNT = 16;

		InputHandler(const ResourceNonOwner<const Window>& window);
		~InputHandler();
		void moveToNextFrame();

		// Cache management
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

		void getJoystickSpeedForGamepad(uint8_t gamepadIdx, uint8_t joystickIdx, float& outX, float& outY, const void* instancePtr = nullptr);
		float getTriggerValueForGamepad(uint8_t gamepadIdx, uint8_t triggerIdx, const void* instancePtr = nullptr);
		// idx 0 -> A, 1 -> B, 2 -> X, 3 -> Y
		bool isGamepadButtonPressed(uint8_t gamepadIdx, uint8_t buttonIdx, const void* instancePtr = nullptr);

		// Callbacks
		void inputHandlerKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		void inputHandlerCharCallback(GLFWwindow* window, unsigned int codepoint);
		void inputHandlerMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
		void inputHandlerScrollCallback(GLFWwindow* window, double offsetX, double offsetY);
		void inputHandlerJoystickCallback(int jid, int event);

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

		static constexpr uint32_t GAMEPAD_JOYSTICK_COUNT = 2;
		static constexpr uint32_t GAMEPAD_TRIGGER_COUNT = 2;
		static constexpr float GAMEPAD_JOYSTICK_DEAD_ZONE_SIZE = 0.15f;
		static constexpr uint32_t GAMEPAD_BUTTON_COUNT = 15;
		struct GamepadCache
		{
			bool isActiveNextFrame = false;
			bool isActive = false;

			struct JoystickEvent
			{
				float offsetX;
				float offsetY;
			};
			JoystickEvent joystickEvent[GAMEPAD_JOYSTICK_COUNT];

			float triggerEvents[GAMEPAD_TRIGGER_COUNT];

			enum class GamepadButtonState { RELEASED = 0, PRESSED = 1 };
			GamepadButtonState buttons[GAMEPAD_BUTTON_COUNT];

			void clear()
			{
				if (!isActive)
					return;

				joystickEvent[0] = joystickEvent[1] = {0.0f, 0.0f};
				triggerEvents[0] = triggerEvents[1] = 0.0f;
				for (GamepadButtonState& buttonState : buttons)
				{
					buttonState = GamepadButtonState::RELEASED;
				}
			}

			void concatenate(const GamepadCache& other)
			{
				isActive = other.isActive;

				if (!isActive)
					return;

				for (uint32_t i = 0; i < GAMEPAD_JOYSTICK_COUNT; ++i)
				{
					joystickEvent[i].offsetX += other.joystickEvent[i].offsetX;
					joystickEvent[i].offsetY += other.joystickEvent[i].offsetY;
				}

				for (uint32_t i = 0; i < GAMEPAD_TRIGGER_COUNT; ++i)
				{
					triggerEvents[i] += other.triggerEvents[i];
				}

				for (uint8_t buttonIdx = 0; buttonIdx < GAMEPAD_BUTTON_COUNT; ++buttonIdx)
				{
					buttons[buttonIdx] = std::max(buttons[buttonIdx], other.buttons[buttonIdx]);
				}
			}
			
		};

		struct AllInputData
		{
			InputCache m_keysCache;
			InputCache m_charCache;
			InputCache m_mouseButtonsCache;
			ScrollCache m_scrollCache;

			GamepadCache m_gamepadCaches[MAX_GAMEPAD_COUNT];

			float m_mousePosX, m_mousePosY;

			void clear()
			{
				m_keysCache.clear();
				m_charCache.clear();
				m_mouseButtonsCache.clear();
				m_scrollCache.clear();

				for (auto& gamepadCache : m_gamepadCaches)
					gamepadCache.clear();
			}

			void concatenate(const AllInputData& other)
			{
				m_keysCache.concatenate(other.m_keysCache);
				m_charCache.concatenate(other.m_charCache);
				m_mouseButtonsCache.concatenate(other.m_mouseButtonsCache);
				m_scrollCache.concatenate(other.m_scrollCache);

				for (uint32_t i = 0; i < MAX_GAMEPAD_COUNT; ++i)
					m_gamepadCaches[i].concatenate(other.m_gamepadCaches[i]);
			}
		} m_data;

		std::map<const void*, std::pair<AllInputData, std::mutex>> m_dataCache;

		ResourceNonOwner<const Window> m_window;
	};
}

#endif
