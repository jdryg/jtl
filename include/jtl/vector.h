#ifndef JTL_VECTOR_H
#define JTL_VECTOR_H

#include <stdint.h>
#include <bx/allocator.h>
#include <bx/sort.h>
#include "jtl.h"

#include <type_traits> // std::is_trivially_XXX, etc.

BX_PRAGMA_DIAGNOSTIC_PUSH()
BX_PRAGMA_DIAGNOSTIC_IGNORED_MSVC(4127) // conditional expression is constant

namespace jtl
{
template<typename T, GetAllocatorFunc A = getDefaultAllocator>
class vector
{
public:
	typedef T* iterator;
	typedef const T* const_iterator;

	vector();
	~vector();

	uint32_t size() const;
	bool empty() const;
	const T& operator [] (uint32_t index) const;
	T& operator[] (uint32_t index);

	void push_back(const T& item);
	void pop_back();

	void reserve(uint32_t capacity);
	void resize(uint32_t sz);
	void clear();

	iterator begin();
	const_iterator begin() const;
	iterator end();
	const_iterator end() const;

	iterator find(const T& item);
	const_iterator find(const T& item) const;

	void sort(bx::ComparisonFn comparator);

	iterator erase(iterator iter);
	iterator erase(iterator first, iterator last);

private:
	T * m_Items;
	uint32_t m_Size;
	uint32_t m_Capacity;
};

template<typename T, GetAllocatorFunc A>
inline vector<T, A>::vector()
	: m_Items(nullptr)
	, m_Size(0)
	, m_Capacity(0)
{
}

template<typename T, GetAllocatorFunc A>
inline vector<T, A>::~vector()
{
	clear();
}

template<typename T, GetAllocatorFunc A>
inline uint32_t vector<T, A>::size() const
{
	return m_Size;
}

template<typename T, GetAllocatorFunc A>
inline bool vector<T, A>::empty() const
{
	return m_Size == 0;
}

template<typename T, GetAllocatorFunc A>
inline const T& vector<T, A>::operator[](uint32_t index) const
{
	JTL_CHECK(index < m_Size, "Invalid index");
	return m_Items[index];
}

template<typename T, GetAllocatorFunc A>
inline T& vector<T, A>::operator[](uint32_t index)
{
	JTL_CHECK(index < m_Size, "Invalid index");
	return m_Items[index];
}

template<typename T, GetAllocatorFunc A>
inline void vector<T, A>::push_back(const T& item)
{
	reserve(m_Size + 1);

	T* dst = &m_Items[m_Size++];

	if (std::is_trivially_copy_constructible<T>::value) {
		bx::memCopy(dst, &item, sizeof(T));
	} else {
		BX_PLACEMENT_NEW(dst, T)(item);
	}
}

template<typename T, GetAllocatorFunc A>
inline void vector<T, A>::pop_back()
{
	JTL_CHECK(m_Size != 0, "Cannot pop_back() from empty vector");

	--m_Size;

	if (!std::is_trivially_destructible<T>::value) {
		m_Items[m_Size].~T();
	}
}

template<typename T, GetAllocatorFunc A>
inline void vector<T, A>::reserve(uint32_t newCapacity)
{
	if (newCapacity > m_Capacity) {
		m_Capacity = (newCapacity & ~31u) + 32; // TODO: grow policy

		bx::AllocatorI* allocator = A();

		T* newItems = (T*)BX_ALLOC(allocator, sizeof(T) * m_Capacity);

		if (m_Items != nullptr) {
			// Copy old items to new array
			if (std::is_trivially_copy_constructible<T>::value) {
				bx::memCopy(newItems, m_Items, sizeof(T) * m_Size);
			} else {
				T* dst = newItems;
				const T* src = m_Items;
				const uint32_t n = m_Size;
				for (uint32_t i = 0; i < n; ++i) {
					BX_PLACEMENT_NEW(dst, T)(*src);
					++dst;
					++src;
				}
			}

			// Destruct old items if neccessary
			if (!std::is_trivially_destructible<T>::value) {
				T* item = m_Items;
				const uint32_t n = m_Size;
				for (uint32_t i = 0; i < n; ++i) {
					item->~T();
					++item;
				}
			}

			// Free old array
			BX_FREE(allocator, m_Items);
		}

		m_Items = newItems;
	}
}

template<typename T, GetAllocatorFunc A>
inline void vector<T, A>::resize(uint32_t sz)
{
	reserve(sz);

	if (sz > m_Size) {
		// Construct all new objects
		if (!std::is_trivially_constructible<T>::value) {
			T* item = &m_Items[m_Size];
			for (uint32_t i = m_Size; i < sz; ++i) {
				BX_PLACEMENT_NEW(item, T)();
				++item;
			}
		}
	} else if (sz < m_Size) {
		// Destruct all extra objects
		if (!std::is_trivially_destructible<T>::value) {
			T* item = &m_Items[sz];
			for (uint32_t i = sz; i < m_Size; ++i) {
				item->~T();
				++item;
			}
		}
	}

	m_Size = sz;
}

template<typename T, GetAllocatorFunc A>
inline void vector<T, A>::clear()
{
	if (!std::is_trivially_destructible<T>::value) {
		T* item = m_Items;
		const uint32_t n = m_Size;
		for (uint32_t i = 0; i < n; ++i) {
			item->~T();
			++item;
		}
	}

	bx::AllocatorI* allocator = A();
	BX_FREE(allocator, m_Items);
	m_Items = nullptr;
	m_Size = 0;
	m_Capacity = 0;
}

template<typename T, GetAllocatorFunc A>
inline typename vector<T, A>::iterator vector<T, A>::begin()
{
	return &m_Items[0];
}

template<typename T, GetAllocatorFunc A>
inline typename vector<T, A>::const_iterator vector<T, A>::begin() const
{
	return &m_Items[0];
}

template<typename T, GetAllocatorFunc A>
inline typename vector<T, A>::iterator vector<T, A>::end()
{
	return &m_Items[m_Size];
}

template<typename T, GetAllocatorFunc A>
inline typename vector<T, A>::const_iterator vector<T, A>::end() const
{
	return &m_Items[m_Size];
}

template<typename T, GetAllocatorFunc A>
inline typename vector<T, A>::iterator vector<T, A>::erase(typename vector<T, A>::iterator iter)
{
	const uint32_t index = (uint32_t)(iter - m_Items);
	JTL_CHECK(index < m_Size, "Invalid iterator");

	const uint32_t numItemsInFront = (m_Size - 1) - index;

	// Destruct the specified item
	if (!std::is_trivially_destructible<T>::value) {
		T* item = &m_Items[index];
		item->~T();
	}

	// If this isn't the last item, move everything in front of it one 
	// position backwards.
	if (numItemsInFront) {
		if (std::is_trivially_destructible<T>::value && std::is_trivially_copyable<T>::value) {
			bx::memMove(&m_Items[index], &m_Items[index + 1], sizeof(T) * numItemsInFront);
		} else {
			const uint32_t n = m_Size - 1;
			T* dst = &m_Items[index];
			T* src = &m_Items[index + 1];
			for (uint32_t i = 0; i < numItemsInFront; ++i) {
				// Copy-construct item i from item i + 1
				BX_PLACEMENT_NEW(dst, T)(*src);

				// Destruct item i + 1
				src->~T();

				// Move on
				++dst;
				++src;
			}
		}
	}

	--m_Size;

	return &m_Items[index];
}

template<typename T, GetAllocatorFunc A>
inline typename vector<T, A>::iterator vector<T, A>::erase(typename vector<T, A>::iterator first, typename vector<T, A>::iterator last)
{
	const uint32_t firstIndex = (uint32_t)(first - m_Items);
	const uint32_t lastIndex = (uint32_t)(last - m_Items);
	JTL_CHECK(firstIndex < m_Size, "Invalid iterator (first)");
	JTL_CHECK(lastIndex <= m_Size, "Invalid iterator (last)");
	JTL_CHECK(firstIndex < lastIndex, "Invalid iterator order (first >= last)");

	// If T is POD (trivially destructible and copyable), we just have to memmove the items
	// after (and including) the last to the first
	if (std::is_trivially_destructible<T>::value && std::is_trivially_copyable<T>::value) {
		if (lastIndex != m_Size) {
			bx::memMove(&m_Items[firstIndex], &m_Items[lastIndex], sizeof(T) * (m_Size - lastIndex));
		}
	} else {
		// T isn't a POD. Copy all items one by one.
		T* dst = &m_Items[firstIndex];
		T* src = &m_Items[lastIndex];
		for (uint32_t i = lastIndex; i < m_Size; ++i) {
			// Destruct erased item
			dst->~T();

			// Copy item from end to this position
			BX_PLACEMENT_NEW(dst, T)(*src);

			// Move on
			++dst;
			++src;
		}

		// Destruct all remaining items.
		const uint32_t destructStart = firstIndex + (m_Size - lastIndex);
		const uint32_t numDestruct = m_Size - destructStart;
		T* item = &m_Items[destructStart];
		for (uint32_t i = destructStart; i < m_Size; ++i) {
			item->~T();
			++item;
		}
	}

	m_Size -= (lastIndex - firstIndex);

	return &m_Items[lastIndex];
}

template<typename T, GetAllocatorFunc A>
inline typename vector<T, A>::iterator vector<T, A>::find(const T& item)
{
	const uint32_t n = m_Size;
	for (uint32_t i = 0; i < n; ++i) {
		if (m_Items[i] == item) {
			return &m_Items[i];
		}
	}

	return end();
}

template<typename T, GetAllocatorFunc A>
inline typename vector<T, A>::const_iterator vector<T, A>::find(const T& item) const
{
	const uint32_t n = m_Size;
	for (uint32_t i = 0; i < n; ++i) {
		if (m_Items[i] == item) {
			return &m_Items[i];
		}
	}

	return end();
}

template<typename T, GetAllocatorFunc A>
inline void vector<T, A>::sort(bx::ComparisonFn comparator)
{
	if (!m_Size || !m_Items) {
		return;
	}

	bx::quickSort(m_Items, m_Size, sizeof(T), comparator);
}
}

BX_PRAGMA_DIAGNOSTIC_POP()

#endif
