#ifndef JTL_HASH_MAP_H
#define JTL_HASH_MAP_H

#include <stdint.h>
#include <bx/allocator.h>
#include "jtl.h"

namespace jtl
{
static const uint32_t kInvalidHashValue = ~0u;

template<typename KeyT
	, typename ValueT
	, GetAllocatorFunc A = getDefaultAllocator
	, typename HasherT = hash<KeyT>
	, typename EqualT = equal<KeyT>>
	class hash_map
{
public:
	typedef hash_map<KeyT, ValueT, A, HasherT, EqualT> this_type;
	typedef pair<KeyT, ValueT> value_type;

	struct iterator
	{
		const this_type* m_HashMap;
		uint32_t m_BucketID;

		iterator()
			: m_HashMap(nullptr)
			, m_BucketID(~0u)
		{
		}

		iterator(const this_type* parent, uint32_t bucketID)
			: m_HashMap(parent)
			, m_BucketID(bucketID)
		{
		}

		iterator& operator++()
		{
			do {
				++m_BucketID;
			} while (m_BucketID < m_HashMap->m_NumBuckets && !m_HashMap->m_Buckets[m_BucketID].m_Filled);

			return *this;
		}

		bool operator == (const iterator& other) const
		{
			return m_BucketID == other.m_BucketID;
		}

		bool operator != (const iterator& other) const
		{
			return m_BucketID != other.m_BucketID;
		}

		pair<KeyT, ValueT>* operator -> ()
		{
			return &m_HashMap->m_Buckets[m_BucketID].m_Pair;
		}
	};

	hash_map();
	~hash_map();

	iterator begin();
	iterator end() const;
	bool empty() const;

	void insert(const pair<KeyT, ValueT>& pair);
	iterator erase(iterator& it);

	iterator find(const KeyT& key) const;

	void clear();
	void reserve(uint32_t n);

private:
	struct Bucket
	{
		pair<KeyT, ValueT> m_Pair;
		uint32_t m_Hash;
		bool m_Filled;
	};

	HasherT m_Hasher;
	EqualT m_Comparator;
	Bucket* m_Buckets;
	uint32_t m_NumBuckets;
	uint32_t m_NumFilledBuckets;
	int m_MaxProbeLength;

	void insert(const KeyT& key, const ValueT& val, uint32_t hash);
};

template<typename KeyT, typename ValueT, GetAllocatorFunc A, typename HasherT, typename EqualT>
inline hash_map<KeyT, ValueT, A, HasherT, EqualT>::hash_map()
	: m_Buckets(nullptr)
	, m_NumBuckets(0)
	, m_NumFilledBuckets(0)
	, m_MaxProbeLength(-1)
{
}

template<typename KeyT, typename ValueT, GetAllocatorFunc A, typename HasherT, typename EqualT>
inline hash_map<KeyT, ValueT, A, HasherT, EqualT>::~hash_map()
{
	clear();
}

template<typename KeyT, typename ValueT, GetAllocatorFunc A, typename HasherT, typename EqualT>
inline typename hash_map<KeyT, ValueT, A, HasherT, EqualT>::iterator hash_map<KeyT, ValueT, A, HasherT, EqualT>::begin()
{
	const uint32_t n = m_NumBuckets;
	for (uint32_t i = 0; i < n; ++i) {
		if (m_Buckets[i].m_Filled) {
			return iterator(this, i);
		}
	}

	return end();
}

template<typename KeyT, typename ValueT, GetAllocatorFunc A, typename HasherT, typename EqualT>
inline typename hash_map<KeyT, ValueT, A, HasherT, EqualT>::iterator hash_map<KeyT, ValueT, A, HasherT, EqualT>::end() const
{
	return iterator(this, m_NumBuckets);
}

template<typename KeyT, typename ValueT, GetAllocatorFunc A, typename HasherT, typename EqualT>
inline bool hash_map<KeyT, ValueT, A, HasherT, EqualT>::empty() const
{
	return m_NumFilledBuckets == 0;
}

template<typename KeyT, typename ValueT, GetAllocatorFunc A, typename HasherT, typename EqualT>
inline void hash_map<KeyT, ValueT, A, HasherT, EqualT>::insert(const pair<KeyT, ValueT>& pair)
{
	const uint32_t hash = m_Hasher(pair.first);
	insert(pair.first, pair.second, hash);
}

template<typename KeyT, typename ValueT, GetAllocatorFunc A, typename HasherT, typename EqualT>
inline typename hash_map<KeyT, ValueT, A, HasherT, EqualT>::iterator hash_map<KeyT, ValueT, A, HasherT, EqualT>::erase(typename hash_map<KeyT, ValueT, A, HasherT, EqualT>::iterator& it)
{
	JTL_CHECK(it.m_HashMap == this, "Invalid iterator");
	JTL_CHECK(it.m_BucketID < m_NumBuckets, "Invalid iterator");

	Bucket* bucket = &m_Buckets[it.m_BucketID];
	bucket->m_Filled = false;
	bucket->m_Hash = kInvalidHashValue;
	bucket->m_Pair.~pair<KeyT, ValueT>();

	--m_NumFilledBuckets;

	return ++it;
}

template<typename KeyT, typename ValueT, GetAllocatorFunc A, typename HasherT, typename EqualT>
inline typename hash_map<KeyT, ValueT, A, HasherT, EqualT>::iterator hash_map<KeyT, ValueT, A, HasherT, EqualT>::find(const KeyT& key) const
{
	if (empty()) {
		return end();
	}

	const uint32_t mask = m_NumBuckets - 1;
	const uint32_t hash = m_Hasher(key);
	for (int offset = 0; offset <= m_MaxProbeLength; ++offset) {
		const uint32_t bucketID = (hash + offset) & mask;
		Bucket* bucket = &m_Buckets[bucketID];
		if (bucket->m_Filled && bucket->m_Hash == hash) {
			JTL_CHECK(m_Comparator(bucket->m_Pair.first, key), "Hash collision!");
			return iterator(this, bucketID);
		}
	}

	return end();
}

template<typename KeyT, typename ValueT, GetAllocatorFunc A, typename HasherT, typename EqualT>
inline void hash_map<KeyT, ValueT, A, HasherT, EqualT>::clear()
{
	// Destruct all pairs in filled buckets
	const uint32_t numBuckets = m_NumBuckets;
	for (uint32_t i = 0; i < numBuckets; ++i) {
		if (m_Buckets[i].m_Filled) {
			m_Buckets[i].m_Pair.~pair<KeyT, ValueT>();
		}
	}

	// Deallocate buckets
	bx::AllocatorI* allocator = A();
	BX_FREE(allocator, m_Buckets);
	m_Buckets = nullptr;
	m_NumBuckets = 0;
	m_NumFilledBuckets = 0;
	m_MaxProbeLength = -1;
}

template<typename KeyT, typename ValueT, GetAllocatorFunc A, typename HasherT, typename EqualT>
inline void hash_map<KeyT, ValueT, A, HasherT, EqualT>::reserve(uint32_t n)
{
	// Code borrowed from https://github.com/emilk/emilib/blob/master/emilib/hash_map.hpp#L493
	uint32_t numRequiredBuckets = n + (n >> 1) + 1;
	if (numRequiredBuckets <= m_NumBuckets) {
		return;
	}

	uint32_t numBuckets = 4;
	while (numBuckets < numRequiredBuckets) {
		numBuckets <<= 1;
	}

	bx::AllocatorI* allocator = A();
	Bucket* newBuckets = (Bucket*)BX_ALLOC(allocator, sizeof(Bucket) * numBuckets);
	JTL_CHECK(newBuckets, "Allocation failed");
	for (uint32_t i = 0; i < numBuckets; ++i) {
		newBuckets[i].m_Hash = kInvalidHashValue;
		newBuckets[i].m_Filled = false;
	}

	// Reinsert everything to the new bucket list
	const uint32_t oldNumBuckets = m_NumBuckets;
	Bucket* oldBuckets = m_Buckets;

	m_Buckets = newBuckets;
	m_NumBuckets = numBuckets;
	m_NumFilledBuckets = 0;
	m_MaxProbeLength = -1;

	for (uint32_t i = 0; i < oldNumBuckets; ++i) {
		if (oldBuckets[i].m_Filled) {
			const pair<KeyT, ValueT>& oldPair = oldBuckets[i].m_Pair;

			insert(oldPair.first, oldPair.second, oldBuckets[i].m_Hash);

			// Destruct old pair.
			oldPair.~pair<KeyT, ValueT>();
		}
	}

	BX_FREE(allocator, oldBuckets);
}

template<typename KeyT, typename ValueT, GetAllocatorFunc A, typename HasherT, typename EqualT>
inline void hash_map<KeyT, ValueT, A, HasherT, EqualT>::insert(const KeyT& key, const ValueT& val, uint32_t hash)
{
	JTL_CHECK(hash != kInvalidHashValue, "FIXME: Hash value equal to kInvalidHashValue");

	reserve(m_NumFilledBuckets + 1);

	const uint32_t mask = m_NumBuckets - 1;
	int offset = 0;
	for (; offset <= m_MaxProbeLength; ++offset) {
		const uint32_t bucketID = (hash + offset) & mask;
		Bucket* bucket = &m_Buckets[bucketID];

		if (bucket->m_Filled) {
			if (hash == bucket->m_Hash) {
				// Hash value are the same. Make sure keys are the same too.
				JTL_CHECK(m_Comparator(bucket->m_Pair.first, key), "Hash collision!");
				JTL_WARN(false, "Key already in hash_map! Replace?");
				return;
			}
		} else {
			// An empty bucket within the max probe length has been found.
			bucket->m_Hash = hash;
			bucket->m_Filled = true;
			BX_PLACEMENT_NEW(&bucket->m_Pair, value_type)(key, val);
			++m_NumFilledBuckets;
			return;
		}
	}

	// No empty bucket found. Continue searching.
	JTL_CHECK(offset == m_MaxProbeLength + 1, "Invalid offset");
	for (;; ++offset) {
		const uint32_t bucketID = (hash + offset) & mask;
		Bucket* bucket = &m_Buckets[bucketID];

		if (!bucket->m_Filled) {
			bucket->m_Hash = hash;
			bucket->m_Filled = true;
			BX_PLACEMENT_NEW(&bucket->m_Pair, value_type)(key, val);

			++m_NumFilledBuckets;
			m_MaxProbeLength = offset;
			break;
		}
	}
}
}

#endif
