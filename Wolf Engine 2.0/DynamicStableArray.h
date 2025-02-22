#pragma once

#include <array>
#include <mutex>
#include <vector>

#include "ResourceUniqueOwner.h"

namespace Wolf
{
	template <class T, size_t BatchSize>
	class DynamicStableArray
	{
	public:
		DynamicStableArray();

		void push_back(const T&& element);
		template <class ... Args>
		T& emplace_back(Args&&... args);
		void resize(size_t newSize);
		void clear();

		size_t size() const;
		bool empty() const;

		void lockAccessElements();
		void unlockAccessElements();

		T& operator [](size_t idx);
		const T& operator [] (size_t idx) const;
		T& back();

	private:
		struct Page
		{
			std::array<T, BatchSize> elements;
			uint8_t count = 0;
		};
		std::vector<std::unique_ptr<Page>> m_pages;

		std::mutex m_accessElementsLock;

		void resizePages(size_t newSize);
		void beforeAddingNewElement();
	};

	template <class T, size_t BatchSize>
	DynamicStableArray<T, BatchSize>::DynamicStableArray()
	{
		resizePages(1);
	}

	template <class T, size_t BatchSize>
	void DynamicStableArray<T, BatchSize>::push_back(const T&& element)
	{
		lockAccessElements();

		beforeAddingNewElement();

		std::unique_ptr<Page>& lastBatch = m_pages.back();
		lastBatch->elements[lastBatch->count++] = element;

		unlockAccessElements();
	}

	template <class T, size_t BatchSize>
	template <class ... Args>
	T& DynamicStableArray<T, BatchSize>::emplace_back(Args&&... args)
	{
		lockAccessElements();

		if constexpr (hasResetFunction<T>)
		{
			beforeAddingNewElement();

			std::unique_ptr<Page>& lastBatch = m_pages.back();
			lastBatch->elements[lastBatch->count++].reset(args...);
		}
		else
		{
			push_back(T(args...));
		}

		unlockAccessElements();

		return back();
	}

	template <class T, size_t BatchSize>
	void DynamicStableArray<T, BatchSize>::resize(size_t newSize)
	{
		lockAccessElements();

		uint32_t newBatchCount = static_cast<uint32_t>(newSize / BatchSize + 1);
		uint32_t newElementCount = static_cast<uint32_t>(newSize % BatchSize);
		if (newBatchCount != m_pages.size())
		{
#ifdef IMMEDIATE_DELETE
			if (newBatchCount < m_pages.size())
			{
				if constexpr (hasResetFunction<T>)
				{
					for (uint32_t elementIdx = newElementCount; elementIdx < BatchSize; ++elementIdx)
					{
						m_pages[newBatchCount - 1].elements[elementIdx].reset();
					}
				}
			}
#endif

			resizePages(newBatchCount);
			m_pages.back()->count = newElementCount;
		}

		if (m_pages.back()->count != newElementCount)
		{
			m_pages.back()->count = newElementCount;
		}

		unlockAccessElements();
	}

	template <class T, size_t BatchSize>
	void DynamicStableArray<T, BatchSize>::clear()
	{
		lockAccessElements();

		m_pages.clear();
		resizePages(1);

		unlockAccessElements();
	}

	template <class T, size_t BatchSize>
	size_t DynamicStableArray<T, BatchSize>::size() const
	{
		return (m_pages.size() - 1) * BatchSize + m_pages.back()->count;
	}

	template <class T, size_t BatchSize>
	bool DynamicStableArray<T, BatchSize>::empty() const
	{
		return m_pages.size() == 1 && m_pages.back()->count == 0;
	}

	template <class T, size_t BatchSize>
	void DynamicStableArray<T, BatchSize>::lockAccessElements()
	{
		m_accessElementsLock.lock();
	}

	template <class T, size_t BatchSize>
	void DynamicStableArray<T, BatchSize>::unlockAccessElements()
	{
		m_accessElementsLock.unlock();
	}

	template <class T, size_t BatchSize>
	T& DynamicStableArray<T, BatchSize>::operator[](size_t idx)
	{
		if (idx >= size())
			Debug::sendCriticalError("Index out of range");

		uint32_t batchIdx = static_cast<uint32_t>(idx / BatchSize);
		uint32_t elementIdx = static_cast<uint32_t>(idx % BatchSize);
		return m_pages[batchIdx]->elements[elementIdx];
	}

	template <class T, size_t BatchSize>
	const T& DynamicStableArray<T, BatchSize>::operator[](size_t idx) const
	{
		uint32_t batchIdx = static_cast<uint32_t>(idx / BatchSize);
		uint32_t elementIdx = static_cast<uint32_t>(idx % BatchSize);
		return m_pages[batchIdx]->elements[elementIdx];
	}

	template <class T, size_t BatchSize>
	T& DynamicStableArray<T, BatchSize>::back()
	{
		if (m_pages.back()->count > 0)
			return m_pages.back()->elements[m_pages.back()->count - 1];
		else
			return m_pages[m_pages.size() - 2]->elements.back();
	}

	template <class T, size_t BatchSize>
	void DynamicStableArray<T, BatchSize>::resizePages(size_t newSize)
	{
		const size_t previousSize = m_pages.size();
		m_pages.resize(newSize);

		for (size_t i = previousSize; i < newSize; ++i)
		{
			m_pages[i].reset(new Page);
		}
	}

	template <class T, size_t BatchSize>
	void DynamicStableArray<T, BatchSize>::beforeAddingNewElement()
	{
		std::unique_ptr<Page>& lastBatch = m_pages.back();
		if (lastBatch->count >= BatchSize)
		{
			resizePages(m_pages.size() + 1);
		}
	}
}
