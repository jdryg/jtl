#ifndef JTL_H
#define JTL_H

#ifndef JTL_CONFIG_DEBUG
#define JTL_CONFIG_DEBUG 0
#endif

#if JTL_CONFIG_DEBUG
#include <bx/debug.h>

#define JTL_TRACE(_format, ...) \
	do { \
		bx::debugPrintf(BX_FILE_LINE_LITERAL "JTL " _format "\n", ##__VA_ARGS__); \
	} while(0)

#define JTL_WARN(_condition, _format, ...) \
	do { \
		if (!(_condition) ) { \
			JTL_TRACE(BX_FILE_LINE_LITERAL _format, ##__VA_ARGS__); \
		} \
	} while(0)

#define JTL_CHECK(_condition, _format, ...) \
	do { \
		if (!(_condition) ) { \
			JTL_TRACE(BX_FILE_LINE_LITERAL _format, ##__VA_ARGS__); \
			bx::debugBreak(); \
		} \
	} while(0)
#else
#define JTL_TRACE(_format, ...)
#define JTL_WARN(_condition, _format, ...)
#define JTL_CHECK(_condition, _format, ...)
#endif

namespace bx
{
struct AllocatorI;
}

namespace jtl
{
typedef bx::AllocatorI* (*GetAllocatorFunc)();

bx::AllocatorI* getDefaultAllocator();

uint32_t fnv1a(const void* buffer, uint32_t len);

template<typename T>
struct equal
{
	bool operator ()(const T& a, const T& b) const
	{
		return a == b;
	}
};

template<typename T>
struct hash
{
	uint32_t operator() (const T& a) const
	{
		return fnv1a(&a, sizeof(T));
	}
};

template<typename FirstT, typename SecondT>
struct pair
{
	FirstT first;
	SecondT second;

	pair(const FirstT& f, const SecondT& s)
		: first(f)
		, second(s)
	{
	}

	~pair()
	{
	}
};
}

#endif
