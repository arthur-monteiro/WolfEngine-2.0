#pragma once

#include <memory>
#ifdef RESOURCE_DEBUG
#include <source_location>
#endif

#include <mutex>

#include "Debug.h"
#include "ResourceNonOwner.h"

namespace Wolf
{
	template<typename U>
	concept hasResetFunction = requires(U object)
	{
		object.reset(nullptr);
	};

	template<typename U>
	concept hasReleaseFunction = requires(U object)
	{
		object.release();
	};

#ifdef RESOURCE_TRACKING
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
		ResourceUniqueOwner& operator=(ResourceUniqueOwner&& other) noexcept
		{
			m_resource = std::move(other.m_resource);
			m_nonOwnedResourceCount = other.m_nonOwnedResourceCount;
#ifdef RESOURCE_DEBUG
			m_nonOwnerResources = std::move(other.m_nonOwnerResources);
#endif

			return *this;
		}
		ResourceUniqueOwner(const ResourceUniqueOwner&) { Wolf::Debug::sendCriticalError("This function shouldn't be called"); }
		~ResourceUniqueOwner();

		void reset(T* resource);
		void transferFrom(ResourceUniqueOwner<T>& src);
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
		[[nodiscard]] T& operator*() { return *m_resource; }

	private:
		friend ResourceUniqueOwner<T>;

		std::unique_ptr<T> m_resource;

		std::mutex m_nonOwnerResourceMutex;
		uint32_t m_nonOwnedResourceCount = 0;

		void nonOwnerResourceDeleteCommon(
#ifdef RESOURCE_DEBUG
			const ResourceNonOwner<T>* instance
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
			ResourceNonOwner<T>* instance
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
		std::vector<ResourceNonOwner<T>*> m_nonOwnerResources;
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
	void ResourceUniqueOwner<T>::transferFrom(ResourceUniqueOwner<T>& src)
	{
		reset(src.m_resource.release());
		m_nonOwnerResources.swap(src.m_nonOwnerResources);
		m_nonOwnedResourceCount = static_cast<uint32_t>(m_nonOwnerResources.size());
		src.m_nonOwnedResourceCount = 0;

		for (ResourceNonOwner<T>* nonOwnerResource : m_nonOwnerResources)
		{
			nonOwnerResource->m_onDelete = [this](ResourceNonOwner<T>* instance) { nonOwnerResourceDeleteTracked(instance); };
			nonOwnerResource->m_onDuplicate = [this](ResourceNonOwner<T>* instance) { addNonOwnerResourceTracked(instance); };
		}
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
				[this] { nonOwnerResourceDeleteCommon(); }, [this] { addNonOwnerResourceCommon(); }
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
		const ResourceNonOwner<T>* instance
#endif
	)
	{
		m_nonOwnerResourceMutex.lock();

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

		m_nonOwnerResourceMutex.unlock();
	}

#ifdef RESOURCE_DEBUG
	template <typename T>
	template <typename U>
	void ResourceUniqueOwner<T>::nonOwnerResourceDeleteTracked(ResourceNonOwner<U>* instance)
	{
		nonOwnerResourceDeleteCommon(reinterpret_cast<ResourceNonOwner<T>*>(instance));
	}

	template <typename T>
	template <typename U>
	void ResourceUniqueOwner<T>::nonOwnerResourceDeleteTracked(ResourceNonOwner<const U>* instance)
	{
		nonOwnerResourceDeleteCommon(reinterpret_cast<ResourceNonOwner<T>*>(instance));
	}
#endif

	template <typename T>
	void ResourceUniqueOwner<T>::addNonOwnerResourceCommon(
#ifdef RESOURCE_DEBUG
		ResourceNonOwner<T>* instance
#endif
	)
	{
		m_nonOwnerResourceMutex.lock();

		m_nonOwnedResourceCount++;
#ifdef RESOURCE_DEBUG
		m_nonOwnerResources.push_back(instance);
#endif

		m_nonOwnerResourceMutex.unlock();
	}

#ifdef RESOURCE_DEBUG
	template <typename T>
	template <typename U>
	void ResourceUniqueOwner<T>::addNonOwnerResourceTracked(ResourceNonOwner<U>* instance)
	{
		addNonOwnerResourceCommon(reinterpret_cast<ResourceNonOwner<T>*>(instance));
	}

	template <typename T>
	template <typename U>
	void ResourceUniqueOwner<T>::addNonOwnerResourceTracked(ResourceNonOwner<const U>* instance)
	{
		addNonOwnerResourceCommon(reinterpret_cast<ResourceNonOwner<T>*>(instance));
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
#else
	template <typename T>
	class ResourceUniqueOwner
	{
	public:
		ResourceUniqueOwner(T* ptr = nullptr) : m_ptr(ptr) {}

		template <typename U = T>
		ResourceNonOwner<U> createNonOwnerResource() { return ResourceNonOwner<U>(dynamic_cast<U*>(m_ptr.get())); }
		ResourceNonOwner<const T> createConstNonOwnerResource() { return ResourceNonOwner<const T>(m_ptr.get()); }

		void reset(T* resource) { m_ptr.reset(resource); }
		T* release() { return m_ptr.release(); }

		void transferFrom(ResourceUniqueOwner<T>& src) { reset(src.release()); }

		[[nodiscard]] explicit operator bool() const { return m_ptr != nullptr; }
		[[nodiscard]] T* operator->() const { return m_ptr.get(); }
		[[nodiscard]] const T& operator*() const { return *m_ptr; }
		[[nodiscard]] T& operator*() { return *m_ptr; }

	private:
		std::unique_ptr<T> m_ptr;
	};
#endif
}