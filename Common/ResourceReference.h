#pragma once

#include <memory>

#include "Debug.h"
#include "ResourceNonOwner.h"

namespace Wolf
{
#ifdef RESOURCE_TRACKING
	template <typename T>
	class ResourceReference
	{
	public:
		ResourceReference(T* resource) { m_resourcePtr = resource; m_resourceContainerType = ResourceContainerType::RAW; }
		ResourceReference(const ResourceNonOwner<T>& resource) { m_resourceNonOwner.reset(new ResourceNonOwner<T>(resource)); m_resourceContainerType = ResourceContainerType::RESOURCE_NON_OWNER; }
		ResourceReference(const ResourceReference<T>& resource)
		{
			initFromOther(resource);
		}

		ResourceReference& operator=(const ResourceReference& resource)
		{
			initFromOther(resource);
			return *this;
		}

		~ResourceReference()
		{
			if (m_resourceContainerType == ResourceContainerType::RESOURCE_NON_OWNER)
				m_resourceNonOwner.reset();
		}

		template <typename U = T>
		[[nodiscard]] U* operator->() const
		{
			switch (m_resourceContainerType)
			{
				case ResourceContainerType::RAW: 
					return static_cast<U*>(m_resourcePtr);
					break;
				case ResourceContainerType::RESOURCE_NON_OWNER:
					return static_cast<U*>(m_resourceNonOwner->operator->());
					break;
				default: 
					Debug::sendCriticalError("Unhandled resource container type");
					return nullptr;
			}
		}

		bool isNull() const { return m_resourceContainerType == ResourceContainerType::RAW && m_resourcePtr == nullptr; }

	private:
		void initFromOther(const ResourceReference<T>& other)
		{
			m_resourceContainerType = other.m_resourceContainerType;

			switch (m_resourceContainerType)
			{
			case ResourceContainerType::RAW:
				m_resourcePtr = other.m_resourcePtr;
				break;
			case ResourceContainerType::RESOURCE_NON_OWNER:
				m_resourceNonOwner.reset(new ResourceNonOwner<T>(*other.m_resourceNonOwner));
				break;
			}
		}


		T* m_resourcePtr = nullptr;
		std::unique_ptr<ResourceNonOwner<T>> m_resourceNonOwner;

		enum class ResourceContainerType { RAW, RESOURCE_NON_OWNER } m_resourceContainerType;
	};
#else
	template<class T> 
	class ResourceReference
	{
	public:
		ResourceReference(T* ptr) : m_ptr(ptr) {}
		ResourceReference(ResourceNonOwner<T> resource) : m_ptr(resource.template operator-><T>()) {}

		template <typename U = T>
		[[nodiscard]] U* operator->() const { return static_cast<U*>(m_ptr); }

		bool isNull() const { return m_ptr == nullptr; }

	private:
		T* m_ptr = nullptr;
	};
#endif
}
