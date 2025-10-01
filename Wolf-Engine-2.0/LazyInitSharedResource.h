#pragma once

#include <functional>
#include <memory>

#include "ResourceUniqueOwner.h"

namespace Wolf
{
	template <class T, class U>
	class LazyInitSharedResource
	{
	public:
		LazyInitSharedResource(const std::function<void(ResourceUniqueOwner<T>&)>& initFunction);
		~LazyInitSharedResource();
		
		static ResourceUniqueOwner<T>& getResource() { return m_data; }

	private:
		inline static ResourceUniqueOwner<T> m_data;
		inline static uint32_t m_userCount = 0;
	};

	template <class T, class U>
	LazyInitSharedResource<T, U>::LazyInitSharedResource(const std::function<void(ResourceUniqueOwner<T>&)>& initFunction)
	{
		m_userCount++;

		if (!m_data)
		{
			initFunction(m_data);
		}
	}

	template <class T, class U>
	LazyInitSharedResource<T, U>::~LazyInitSharedResource()
	{
		m_userCount--;

		if (m_userCount == 0)
		{
			m_data.reset(nullptr);
		}
	}
}
