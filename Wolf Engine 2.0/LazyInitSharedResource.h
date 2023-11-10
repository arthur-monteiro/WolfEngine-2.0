#pragma once

#include <functional>
#include <memory>

namespace Wolf
{
	template <class T, class U>
	class LazyInitSharedResource
	{
	public:
		LazyInitSharedResource(const std::function<void(std::unique_ptr<T>&)>& initFunction);
		~LazyInitSharedResource();

		T* getResource() { return m_data.get(); }

	private:
		inline static std::unique_ptr<T> m_data;
		inline static uint32_t m_userCount = 0;
	};

	template <class T, class U>
	LazyInitSharedResource<T, U>::LazyInitSharedResource(const std::function<void(std::unique_ptr<T>&)>& initFunction)
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
