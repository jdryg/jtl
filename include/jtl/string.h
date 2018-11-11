#ifndef JTL_STRING_H
#define JTL_STRING_H

#include <stdint.h>
#include <bx/allocator.h>
#include <bx/string.h>
#include <ctype.h> // tolower()
#include "jtl.h"

namespace jtl
{
class string
{
public:
	string(bx::AllocatorI* allocator = nullptr);
	explicit string(const char* str, bx::AllocatorI* allocator = nullptr);
	explicit string(const char* str, uint32_t len, bx::AllocatorI* allocator = nullptr);
	string(const string& other);
	string(string&& other);

	~string();

	const char* c_str() const;

	uint32_t size() const;
	bool empty() const;

	const char& operator [] (uint32_t index) const;
	char& operator [] (uint32_t index);

	string& operator = (const string& other);

	void push_back(char ch);
	void insert(uint32_t pos, const char* str);

	void sprintf(const char* fmt, ...);

	void assign(const char* str);
	void assign(const char* str, uint32_t len);
	void append(const char* str);
	void append(const char* first, const char* last);
	void append(const string& str);

	void resize(uint32_t sz);
	void reserve(uint32_t capacity);
	void erase(uint32_t pos, uint32_t len);
	void clear();

	void tolower();
	string substr(uint32_t first, uint32_t last) const;
	uint32_t find_last_of(const char* charSet) const;

private:
	char* m_String;
	bx::AllocatorI* m_Allocator;
	uint32_t m_Size;
	uint32_t m_Capacity;
};

inline string::string(bx::AllocatorI* allocator)
	: m_String(nullptr)
	, m_Allocator(allocator ? allocator : getDefaultAllocator())
	, m_Size(0)
	, m_Capacity(0)
{
}

inline string::string(const char* str, bx::AllocatorI* allocator)
	: m_String(nullptr)
	, m_Allocator(allocator ? allocator : getDefaultAllocator())
	, m_Size(0)
	, m_Capacity(0)
{
	assign(str);
}

inline string::string(const char* str, uint32_t len, bx::AllocatorI* allocator)
	: m_String(nullptr)
	, m_Allocator(allocator ? allocator : getDefaultAllocator())
	, m_Size(0)
	, m_Capacity(0)
{
	assign(str, len);
}

inline string::string(const string& other)
	: m_String(nullptr)
	, m_Allocator(other.m_Allocator)
	, m_Size(0)
	, m_Capacity(0)
{
	assign(other.c_str(), other.size());
}

inline string::string(string&& other)
	: m_String(other.m_String)
	, m_Allocator(other.m_Allocator)
	, m_Size(other.m_Size)
	, m_Capacity(other.m_Capacity)
{
	other.m_String = nullptr;
	other.m_Size = 0;
	other.m_Capacity = 0;
}

inline string::~string()
{
	clear();
}

inline const char* string::c_str() const
{
	return m_String;
}

inline uint32_t string::size() const
{
	return m_Size;
}

inline bool string::empty() const
{
	return m_Size == 0;
}

inline const char& string::operator[](uint32_t index) const
{
	JTL_CHECK(index < m_Size, "Invalid index");
	return m_String[index];
}

inline char& string::operator[](uint32_t index)
{
	JTL_CHECK(index < m_Size, "Invalid index");
	return m_String[index];
}

inline string& string::operator = (const string& other)
{
	resize(0);
	assign(other.c_str(), other.size());
	return *this;
}

inline void string::append(const char* str)
{
	const uint32_t len = bx::strLen(str);
	append(str, str + len);
}

inline void string::append(const char* first, const char* last)
{
	const uint32_t len = (uint32_t)(last - first);
	reserve(m_Size + len + 1);
	bx::memCopy(&m_String[m_Size], first, sizeof(char) * len);
	m_Size += len;
	m_String[m_Size] = char(0);
}

inline void string::append(const string& str)
{
	const char* cstr = str.c_str();
	append(cstr, cstr + str.size());
}

inline void string::assign(const char* str)
{
	const uint32_t len = bx::strLen(str);
	assign(str, len);
}

inline void string::assign(const char* str, uint32_t len)
{
	reserve(len + 1);
	bx::memCopy(m_String, str, sizeof(char) * len);
	m_String[len] = char(0);
	m_Size = len;
}

inline void string::reserve(uint32_t newCapacity)
{
	newCapacity++; // HACK: make sure there's space for the null character
	if (newCapacity > m_Capacity) {
		const uint32_t cap = (newCapacity & ~15) + ((newCapacity & 15) ? 16 : 0);
		m_String = (char*)BX_REALLOC(m_Allocator, m_String, sizeof(char) * cap);
		m_Capacity = cap;

		JTL_CHECK(m_Size < m_Capacity, "Invalid size!");
		m_String[m_Size] = 0; // Just to be sure
	}
}

inline void string::clear()
{
	BX_FREE(m_Allocator, m_String);
	m_String = nullptr;
	m_Size = 0;
	m_Capacity = 0;
}

inline void string::resize(uint32_t sz)
{
	reserve(sz + 1);
	m_String[sz] = char(0);
	m_Size = sz;
}

inline void string::push_back(char ch)
{
	reserve(m_Size + 2);
	m_String[m_Size++] = ch;
	m_String[m_Size] = char(0);
}

inline void string::erase(uint32_t first, uint32_t last)
{
	JTL_CHECK(first < m_Size, "Invalid index (first)");
	JTL_CHECK(last <= m_Size, "Invalid index (last)");
	JTL_CHECK(first < last, "Invalid index order (first >= last)");

	const uint32_t sizeAfterLast = m_Size - last;
	if (sizeAfterLast) {
		bx::memMove(&m_String[first], &m_String[last], sizeof(char) * sizeAfterLast);
	}
	m_Size -= (last - first);
	m_String[m_Size] = char(0);
}

inline void string::insert(uint32_t pos, const char* str)
{
	JTL_CHECK(pos < m_Size, "Invalid index");
	JTL_CHECK(str, "str is null");

	const uint32_t len = bx::strLen(str);
	if (!len) {
		return;
	}

	reserve(m_Size + len + 1);
	if (pos != m_Size - 1) {
		bx::memMove(&m_String[pos + len], &m_String[pos], sizeof(char) * (m_Size - pos));
	}
	bx::memCopy(&m_String[pos], str, sizeof(char) * len);
	m_Size += len;
	m_String[m_Size] = char(0);
}

inline void string::sprintf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int len = bx::vsnprintf(nullptr, 0, fmt, args);
	resize(len);
	bx::vsnprintf(m_String, m_Size + 1, fmt, args);
	va_end(args);
}

inline string string::substr(uint32_t first, uint32_t last) const
{
	JTL_CHECK(first < m_Size, "Invalid index (first)");
	JTL_CHECK(last <= m_Size, "Invalid index (last)");
	JTL_CHECK(first < last, "Invalid index order (first >= last)");
	return string(&m_String[first], last - first, m_Allocator);
}

inline uint32_t string::find_last_of(const char* charSet) const
{
	const uint32_t numChars = bx::strLen(charSet);

	bx::StringView sv(m_String, m_Size);

	bool found = false;
	uint32_t maxPos = 0;
	for (uint32_t i = 0; i < numChars; ++i) {
		bx::StringView ptr = bx::strRFind(sv, charSet[i]);
		if (!ptr.isEmpty()) {
			const uint32_t pos = (uint32_t)(ptr.getPtr() - m_String);
			if (pos > maxPos) {
				maxPos = pos;
				found = true;
			}
		}
	}

	return found ? maxPos : ~0u;
}

inline void string::tolower()
{
	const uint32_t len = m_Size;

	const char* cstr = m_String;
	for (uint32_t i = 0; i < len; ++i) {
		m_String[i] = (char)::tolower(*cstr++);
	}
}

inline string to_string(int value)
{
	string str;
	str.sprintf("%d", value);
	return str;
}

inline string to_string(uint32_t value)
{
	string str;
	str.sprintf("%u", value);
	return str;
}

inline string to_string(float value)
{
	string str;
	str.sprintf("%f", value);
	return str;
}
}

#endif
