#pragma once

#include <memory>
#ifdef RESOURCE_DEBUG
#include <source_location>
#endif

#include "Debug.h"
#include "ResourceNonOwner.h"

namespace Wolf
{
	template <typename T>
	class ResourceUniqueOwner
	{
	public:
		ResourceUniqueOwner(T* resource = nullptr) : m_resource(resource) {}
		ResourceUniqueOwner(ResourceUniqueOwner&& other) noexcept : m_resource(std::move(other.m_resource)), m_nonOwnedResourceCount(other.m_nonOwnedResourceCount)
#ifdef RESOURCE_DEBUG
		, m_nonOwnerResources(other.m_nonOwnerResources)
#endif
		{}
		~ResourceUniqueOwner();

		void reset(T* resource);
		T* release();

		template <typename U = T>
		ResourceNonOwner<U> createNonOwnerResource(
#ifdef RESOURCE_DEBUG
			const std::source_location& location = std::source_location::current()
#endif
		);
		ResourceNonOwner<const T> createConstNonOwnerResource(
#ifdef RESOURCE_DEBUG
			const std::source_location& location = std::source_location::current()
#endif
		);

		[[nodiscard]] explicit operator bool() const;
		[[nodiscard]] T* operator->() const { return m_resource.get(); }
		[[nodiscard]] const T& operator*() const { return *m_resource; }

	private:
		std::unique_ptr<T> m_resource;

		uint32_t m_nonOwnedResourceCount = 0;

		void nonOwnerResourceDeleteCommon(
#ifdef RESOURCE_DEBUG
			const ResourceNonOwner<const void>* instance
#endif
		);
#ifdef RESOURCE_DEBUG
		template <typename U = T>
		void nonOwnerResourceDeleteTracked(ResourceNonOwner<U>* instance);
		template <typename U = T>
		void nonOwnerResourceDeleteTracked(ResourceNonOwner<const U>* instance);
#endif

		void addNonOwnerResourceCommon(
#ifdef RESOURCE_DEBUG
			ResourceNonOwner<const void>* instance
#endif
		);
#ifdef RESOURCE_DEBUG
		template <typename U = T>
		void addNonOwnerResourceTracked(ResourceNonOwner<U>* instance);
		template <typename U = T>
		void addNonOwnerResourceTracked(ResourceNonOwner<const U>* instance);
#endif
		void checksBeforeDelete() const;

#ifdef RESOURCE_DEBUG
		std::vector<ResourceNonOwner<const void>*> m_nonOwnerResources;
#endif
	};

	template <typename T>
	ResourceUniqueOwner<T>::~ResourceUniqueOwner()
	{
		if (m_resource)
			checksBeforeDelete();
	}

	template <typename T>
	void ResourceUniqueOwner<T>::reset(T* resource)
	{
		if (m_resource)
		{
			checksBeforeDelete();
		}
		m_resource.reset(resource);
	}

	template <typename T>
	T* ResourceUniqueOwner<T>::release()
	{
		checksBeforeDelete();
		return m_resource.release();
	}

	template <typename T>
	template <typename U>
	ResourceNonOwner<U> ResourceUniqueOwner<T>::createNonOwnerResource(
#ifdef RESOURCE_DEBUG
		const std::source_location& location
#endif
	)
	{
		if (U* resourceAsNewType = dynamic_cast<U*>(m_resource.get()))
		{
			return ResourceNonOwner<U>(resourceAsNewType,
#ifdef RESOURCE_DEBUG
				[this](ResourceNonOwner<U>* instance) { nonOwnerResourceDeleteTracked(instance); }, [this](ResourceNonOwner<U>* instance) { addNonOwnerResourceTracked(instance); }, location
#else
				[this]{ nonOwnerResourceDeleteCommon(); }, [this] { addNonOwnerResourceCommon(); }
#endif
			);
		}
		else
		{
			return ResourceNonOwner<U>(nullptr,
#ifdef RESOURCE_DEBUG
				[this](ResourceNonOwner<U>*) { }, [this](ResourceNonOwner<U>* instance) { }, location
#else
				[this]{ nonOwnerResourceDeleteCommon(); }, [this] { addNonOwnerResourceCommon(); }
#endif
			);
		}
	}

	template <typename T>
	ResourceNonOwner<const T> ResourceUniqueOwner<T>::createConstNonOwnerResource(
#ifdef RESOURCE_DEBUG
		const std::source_location& location
#endif
	)
	{
		return ResourceNonOwner<const T>(m_resource.get(),
#ifdef RESOURCE_DEBUG
			[this] (ResourceNonOwner<const T>* instance) { nonOwnerResourceDeleteTracked(instance); }, [this] (ResourceNonOwner<const T>* instance) { addNonOwnerResourceTracked(instance); }, location
#else
			[this] { nonOwnerResourceDeleteCommon(); }, [this] { addNonOwnerResourceCommon(); }
#endif
		);
	}

	template <typename T>
	ResourceUniqueOwner<T>::operator bool() const
	{
		return static_cast<bool>(m_resource);
	}

	template <typename T>
	void ResourceUniqueOwner<T>::nonOwnerResourceDeleteCommon(
#ifdef RESOURCE_DEBUG
		const ResourceNonOwner<const void>* instance
#endif
	)
	{
		if (m_nonOwnedResourceCount == 0)
			Debug::sendCriticalError("Wrong resource count");
		m_nonOwnedResourceCount--;

#ifdef RESOURCE_DEBUG
		for (int32_t i = static_cast<int32_t>(m_nonOwnerResources.size()) - 1; i >= 0; --i)
		{
			if (m_nonOwnerResources[i] == instance)
			{
				m_nonOwnerResources.erase(m_nonOwnerResources.begin() + i);
				break;
			}
		}
#endif
	}

#ifdef RESOURCE_DEBUG
	template <typename T>
	template <typename U>
	void ResourceUniqueOwner<T>::nonOwnerResourceDeleteTracked(ResourceNonOwner<U>* instance)
	{
		nonOwnerResourceDeleteCommon(reinterpret_cast<ResourceNonOwner<const void>*>(instance));
	}

	template <typename T>
	template <typename U>
	void ResourceUniqueOwner<T>::nonOwnerResourceDeleteTracked(ResourceNonOwner<const U>* instance)
	{
		nonOwnerResourceDeleteCommon(reinterpret_cast<ResourceNonOwner<const void>*>(instance));
	}
#endif

	template <typename T>
	void ResourceUniqueOwner<T>::addNonOwnerResourceCommon(
#ifdef RESOURCE_DEBUG
		ResourceNonOwner<const void>* instance
#endif
	)
	{
		m_nonOwnedResourceCount++;
#ifdef RESOURCE_DEBUG
		m_nonOwnerResources.push_back(instance);
#endif
	}

#ifdef RESOURCE_DEBUG
	template <typename T>
	template <typename U>
	void ResourceUniqueOwner<T>::addNonOwnerResourceTracked(ResourceNonOwner<U>* instance)
	{
		addNonOwnerResourceCommon(reinterpret_cast<ResourceNonOwner<const void>*>(instance));
	}

	template <typename T>
	template <typename U>
	void ResourceUniqueOwner<T>::addNonOwnerResourceTracked(ResourceNonOwner<const U>* instance)
	{
		addNonOwnerResourceCommon(reinterpret_cast<ResourceNonOwner<const void>*>(instance));
	}
#endif

	template <typename T>
	void ResourceUniqueOwner<T>::checksBeforeDelete() const
	{
		if (m_nonOwnedResourceCount > 0)
		{
#ifdef RESOURCE_DEBUG
			__debugbreak();
#endif
			Debug::sendError("Deleting a resource currently used by others");
		}
	}
}