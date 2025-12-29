#pragma once

#include <functional>
#include <memory>
#ifdef RESOURCE_DEBUG
#include <source_location>
#endif

#include "Debug.h"

namespace Wolf
{
#ifdef RESOURCE_TRACKING
	template <typename T>
	class ResourceUniqueOwner;

	template <typename T>
	class NullableResourceNonOwner;

	template <typename T>
	class ResourceNonOwner
	{
	public:
		ResourceNonOwner(T* resource,
#ifdef RESOURCE_DEBUG
			const std::function<void(ResourceNonOwner<T>* instance)>& onDelete, const std::function<void(ResourceNonOwner<T>* instance)>& onDuplicate, const std::source_location& sourceLocation)
#else
			const std::function<void()>& onDelete, const std::function<void()>& onDuplicate)
#endif
			: m_resource(resource), m_onDelete(onDelete), m_onDuplicate(onDuplicate)
		{
#ifdef RESOURCE_DEBUG
			m_sourceLocation.push_back(sourceLocation);
#endif

			m_onDuplicate(
#ifdef RESOURCE_DEBUG
				this
#endif
			); // add instance to parent
		}

		ResourceNonOwner(const ResourceNonOwner& other
#ifdef RESOURCE_DEBUG
			, const std::source_location& sourceLocation = std::source_location::current()
#endif
		)
		{
			m_resource = other.m_resource;
			m_onDelete = other.m_onDelete;
			m_onDuplicate = other.m_onDuplicate;

			m_onDuplicate(
#ifdef RESOURCE_DEBUG
				this
#endif
			);

#ifdef RESOURCE_DEBUG
			m_sourceLocation = other.m_sourceLocation;
			m_sourceLocation.push_back(sourceLocation);
#endif
		}
		ResourceNonOwner(ResourceNonOwner&& other
#ifdef RESOURCE_DEBUG
			, const std::source_location& sourceLocation = std::source_location::current()
#endif
		) noexcept
		{
			m_resource = std::move(other.m_resource);
			m_onDelete = other.m_onDelete; // keep function to origin as it'll need it to destruct itself
			m_onDuplicate = std::move(other.m_onDuplicate);
#ifdef RESOURCE_DEBUG
			m_sourceLocation = std::move(other.m_sourceLocation);
			m_sourceLocation.push_back(sourceLocation);
#endif

			m_onDuplicate(
#ifdef RESOURCE_DEBUG
				this
#endif
			);
		}

		ResourceNonOwner(const NullableResourceNonOwner<T>& nullableResource
#ifdef RESOURCE_DEBUG
			, const std::source_location& sourceLocation = std::source_location::current()
#endif
		);

		[[nodiscard]] T* operator->() const { return m_resource; }
		ResourceNonOwner<T>& operator=(const ResourceNonOwner<T>& other)
		{
			m_onDelete(
#ifdef RESOURCE_DEBUG
				this
#endif
			);

			m_resource = other.m_resource;
			m_onDelete = other.m_onDelete;
			m_onDuplicate = other.m_onDuplicate;
#ifdef RESOURCE_DEBUG
			m_sourceLocation.clear();
			m_sourceLocation.insert(m_sourceLocation.end(), other.m_sourceLocation.begin(), other.m_sourceLocation.end());
			m_sourceLocation.push_back(std::source_location::current());
#endif

			m_onDuplicate(
#ifdef RESOURCE_DEBUG
				this
#endif
			);

			return *this;
		}

		[[nodiscard]] operator bool() const { return m_resource != nullptr; }
		[[nodiscard]] bool operator==(const ResourceNonOwner& other) const { return m_resource == other.m_resource; }
		[[nodiscard]] bool operator==(const NullableResourceNonOwner<T>& other) const { return operator==(other.m_resource); }

		[[nodiscard]] const T& operator*() const noexcept requires(!std::is_void_v<T>) { return *m_resource; }
		[[nodiscard]] T& operator*() requires(!std::is_void_v<T>) { return *m_resource; }

		[[nodiscard]] bool isSame(T* resource) const { return m_resource == resource; }

		~ResourceNonOwner();

	private:
		friend ResourceUniqueOwner<T>;

		T* m_resource;

#ifdef RESOURCE_DEBUG
		std::vector<std::source_location> m_sourceLocation;
		std::function<void(ResourceNonOwner<T>* instance)> m_onDelete;
		std::function<void(ResourceNonOwner<T>* instance)> m_onDuplicate;
#else
		std::function<void()> m_onDelete;
		std::function<void()> m_onDuplicate;
#endif		
	};

	template <typename T>
	ResourceNonOwner<T>::~ResourceNonOwner()
	{
		m_onDelete(
#ifdef RESOURCE_DEBUG
			this
#endif
		);
	}

	template <typename T>
	class NullableResourceNonOwner
	{
	public:
		NullableResourceNonOwner(const ResourceNonOwner<T>& resource) : m_resource(resource)
		{
		}
		NullableResourceNonOwner(const NullableResourceNonOwner<T>& other) : m_resource(other.m_resource)
		{
		}
		NullableResourceNonOwner() : m_resource(static_cast<T*>(nullptr),
#ifdef RESOURCE_DEBUG
			[](ResourceNonOwner<T>* instance) {}, [](ResourceNonOwner<T>* instance) {}, std::source_location::current()
#else
			[]{}, [] {}
#endif
		)
		{
		}

		[[nodiscard]] operator bool() const { return static_cast<bool>(m_resource); }
		[[nodiscard]] const T& operator*() const noexcept requires(!std::is_void_v<T>)
		{
			if (!m_resource)
			{
				Debug::sendCriticalError("Trying to get reference of nullptr");
			}
			return *m_resource;
		}
		[[nodiscard]] T& operator*() requires(!std::is_void_v<T>)
		{
			if (!m_resource)
			{
				Debug::sendCriticalError("Trying to get reference of nullptr");
			}
			return *m_resource;
		}
		[[nodiscard]] T* operator->() const
		{
			if (!m_resource)
			{
				Debug::sendCriticalError("Trying to get reference of nullptr");
			}
			return m_resource.operator->();
		}

		void release()
		{
			m_resource = ResourceNonOwner<T>(static_cast<T*>(nullptr),
#ifdef RESOURCE_DEBUG
				[](ResourceNonOwner<T>* instance) {}, [](ResourceNonOwner<T>* instance) {}, std::source_location::current()
#else
				[]{}, [] {}
#endif
			);
		}

	private:
		friend ResourceNonOwner<T>;

		ResourceNonOwner<T> m_resource;
	};

	template <typename T>
	ResourceNonOwner<T>::ResourceNonOwner(const NullableResourceNonOwner<T>& nullableResource
#ifdef RESOURCE_DEBUG
		, const std::source_location& sourceLocation
#endif
	) : ResourceNonOwner(nullableResource.m_resource
#ifdef RESOURCE_DEBUG
		, sourceLocation
#endif
	)
	{
		if (!nullableResource)
		{
			Debug::sendCriticalError("Trying to create a ResourceNonOwner with nullptr");
		}
	}
#else
	template<class T>
	class ResourceNonOwner
	{
	public:
		ResourceNonOwner(T* ptr = nullptr) : m_ptr(ptr) {}

		template <typename U = T>
		ResourceNonOwner<U> duplicateAs() const { return ResourceNonOwner<U>(static_cast<U*>(m_ptr)); }

		template <typename U = T>
		[[nodiscard]] U* operator->() const { return static_cast<U*>(m_ptr); }

		[[nodiscard]] const T& operator*() const noexcept requires(!std::is_void_v<T>) { return *m_ptr; }
		[[nodiscard]] T& operator*() requires(!std::is_void_v<T>) { return *m_ptr; }

		[[nodiscard]] bool isSame(T* resource) const { return m_ptr == resource; }
		[[nodiscard]] operator bool() const { return m_ptr != nullptr; }

		[[nodiscard]] bool operator==(const ResourceNonOwner& other) const { return m_ptr == other.m_ptr; }

		void release() { m_ptr = nullptr; }

	private:
		T* m_ptr = nullptr;
	};

	template<class T> using NullableResourceNonOwner = ResourceNonOwner<T>;
#endif
}

