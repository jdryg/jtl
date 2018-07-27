#include <bx/allocator.h>

namespace jtl
{
#define FNV_32_PRIME 0x01000193u
#define	FNV1_32_INIT 0x811C9DC5u

// TODO: Configurable default allocator.
bx::AllocatorI* getDefaultAllocator()
{
	static bx::DefaultAllocator defaultAllocator;
	return &defaultAllocator;
}

uint32_t fnv1a(const void* buffer, uint32_t len)
{
	const uint8_t* s = (const uint8_t*)buffer;

	uint32_t hval = FNV1_32_INIT;
	while (len-- > 0) {
		hval ^= (uint32_t)*s++;
#if 1
		hval *= FNV_32_PRIME;
#else
		hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24);
#endif
	}

	return hval;
}
}
