#pragma once

#include <functional>
#ifdef RESOURCE_DEBUG
#include <source_location>
#endif

namespace Wolf
{
	template <typename T>
	class ResourceUniqueOwner;

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

		~ResourceNonOwner();

	private:
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
}
