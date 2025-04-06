#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace Wolf
{
	class Debug
	{
	public:
		enum class Severity { ERROR, WARNING, INFO, VERBOSE };
		enum class Type { WOLF, VULKAN };

		static void sendError(const std::string& errorMessage)
		{
			m_callbackMutex.lock();
			m_callback(Severity::ERROR, Type::WOLF, errorMessage);
			m_callbackMutex.unlock();
		}
		static void sendWarning(const std::string& warningMessage)
		{
			m_callbackMutex.lock();
			m_callback(Severity::WARNING, Type::WOLF, warningMessage);
			m_callbackMutex.unlock();
		}
		static void sendInfo(const std::string& infoMessage)
		{
			m_callbackMutex.lock();
			m_callback(Severity::INFO, Type::WOLF, infoMessage);
			m_callbackMutex.unlock();
		}
		static void sendCriticalError(const std::string& errorMessage)
		{
#ifdef __ANDROID__
			raise(SIGTRAP);
#else
			__debugbreak();
#endif
		}
		static void sendVulkanMessage(const std::string& message, Severity severity)
		{
			m_callbackMutex.lock();
			m_callback(severity, Type::VULKAN, message);
			m_callbackMutex.unlock();
		}
		static void sendMessageOnce(const std::string& message, Severity severity, const void* instancePtr);

		static void setCallback(const std::function<void(Severity, Type, const std::string&)>& callback)
		{
			m_callback = callback;
		}

	private:
		Debug() = default;
		~Debug() = default;

		inline static std::function<void(Severity, Type, const std::string&)> m_callback;
		inline static std::mutex m_callbackMutex;
		inline static std::vector<uint64_t> m_alreadySentHashes;
	};
}
