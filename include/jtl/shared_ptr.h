#ifndef JTL_SHARED_PTR_H
#define JTL_SHARED_PTR_H

#include <stdint.h>
#include <bx/allocator.h>
#include <bx/cpu.h>
#include "jtl.h"

#include <utility> // std::forward
#include <type_traits> // std::aligned_storage

namespace jtl
{
template<typename T>
struct RefCount
{
	typedef typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type StorageT;

	StorageT m_Memory;
	bx::AllocatorI* m_Allocator;
	int m_Count;

	template <typename... Args>
	RefCount(bx::AllocatorI* allocator, Args&&... args)
		: m_Allocator(allocator)
		, m_Count(1)
	{
		BX_PLACEMENT_NEW(&m_Memory, T)(std::forward<Args>(args)...);
	}

	void addRef()
	{
		bx::atomicAddAndFetch(&m_Count, 1);
	}

	void release()
	{
		JTL_CHECK(m_Count > 0, "Invalid reference count");
		if (bx::atomicSubAndFetch(&m_Count, 1) == 0) {
			T* inst = static_cast<T*>(static_cast<void*>(&m_Memory));
			inst->~T();

			bx::AllocatorI* allocator = m_Allocator;
			BX_FREE(allocator, this);
		}
	}
};

template<typename T>
class shared_ptr
{
public:
	shared_ptr();
	shared_ptr(const shared_ptr& other);
	shared_ptr(shared_ptr&& other);
	shared_ptr(std::nullptr_t);
	~shared_ptr();

	T* operator -> () const;

	operator bool() const;

	shared_ptr<T>& operator = (const shared_ptr<T>& other);
	shared_ptr<T>& operator = (shared_ptr<T>&& other);
	void swap(shared_ptr<T>& other);

protected:
	template <typename R>
	friend void allocate_shared_helper(shared_ptr<R>&, RefCount<R>*, R*);

	T* m_Value;
	RefCount<T>* m_RefCount;
};

template<typename T>
inline shared_ptr<T>::shared_ptr()
	: m_Value(nullptr)
	, m_RefCount(nullptr)
{
}

template<typename T>
inline shared_ptr<T>::shared_ptr(const shared_ptr<T>& other)
	: m_Value(other.m_Value)
	, m_RefCount(other.m_RefCount)
{
	if (m_RefCount) {
		m_RefCount->addRef();
	}
}

template<typename T>
inline shared_ptr<T>::shared_ptr(shared_ptr<T>&& other)
	: m_Value(other.m_Value)
	, m_RefCount(other.m_RefCount)
{
	other.m_Value = nullptr;
	other.m_RefCount = nullptr;
}

template<typename T>
inline shared_ptr<T>::shared_ptr(std::nullptr_t)
	: m_Value(nullptr)
	, m_RefCount(nullptr)
{
}

template<typename T>
inline shared_ptr<T>::~shared_ptr()
{
	if (m_RefCount) {
		m_RefCount->release();
	}
}

template<typename T>
inline T* shared_ptr<T>::operator->() const
{
	return m_Value;
}

template<typename T>
inline shared_ptr<T>::operator bool() const
{
	return m_Value != nullptr;
}

template<typename T>
inline shared_ptr<T>& shared_ptr<T>::operator=(const shared_ptr<T>& other)
{
	if (&other != this) {
		shared_ptr<T>(other).swap(*this);
	}

	return *this;
}

template<typename T>
inline shared_ptr<T>& shared_ptr<T>::operator=(shared_ptr<T>&& other)
{
	if (&other != this) {
		shared_ptr<T>(std::move(other)).swap(*this);
	}

	return *this;
}

template<typename T>
inline void shared_ptr<T>::swap(shared_ptr<T>& other)
{
	T* const value = other.m_Value;
	other.m_Value = m_Value;
	m_Value = value;

	RefCount<T>* const refCount = other.m_RefCount;
	other.m_RefCount = m_RefCount;
	m_RefCount = refCount;
}

template <typename R>
inline void allocate_shared_helper(shared_ptr<R>& sharedPtr, RefCount<R>* refCount, R* value)
{
	sharedPtr.m_RefCount = refCount;
	sharedPtr.m_Value = value;
}

template <typename T, typename... Args>
inline shared_ptr<T> allocate_shared(bx::AllocatorI* allocator, Args&&... args)
{
	typedef RefCount<T> RefCountT;

	shared_ptr<T> ret;
	void* mem = BX_ALLOC(allocator, sizeof(RefCountT));
	if (mem) {
		RefCountT* refCount = BX_PLACEMENT_NEW(mem, RefCountT)(allocator, std::forward<Args>(args)...);
		allocate_shared_helper(ret, refCount, static_cast<T*>(static_cast<void*>(&refCount->m_Memory)));
	}

	return ret;
}

template <typename T, typename... Args>
inline shared_ptr<T> make_shared(Args&&... args)
{
	return allocate_shared<T>(getDefaultAllocator(), std::forward<Args>(args)...);
}

template <typename T, typename... Args>
inline shared_ptr<T> make_shared(bx::AllocatorI* allocator, Args&&... args)
{
	return allocate_shared<T>(allocator, std::forward<Args>(args)...);
}
}

#endif
