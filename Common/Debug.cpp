#include "Debug.h"

#include <xxh64.hpp>

void Wolf::Debug::sendMessageOnce(const std::string& message, Severity severity, const void* instancePtr)
{
	const uint64_t messageHash = xxh64::hash(message.data(), message.size(), 0);
	const uint64_t severityHash = static_cast<uint64_t>(severity) << sizeof(void*);
	const uint64_t instancePtrHash = reinterpret_cast<std::uintptr_t>(instancePtr);
	const uint64_t hash = messageHash | severityHash | instancePtrHash;
	if (std::find(m_alreadySentHashes.begin(), m_alreadySentHashes.end(), hash) == m_alreadySentHashes.end())
	{
		m_callbackMutex.lock();

		m_callback(severity, Type::WOLF, message);
		m_alreadySentHashes.push_back(hash);

		m_callbackMutex.unlock();
	}
}
