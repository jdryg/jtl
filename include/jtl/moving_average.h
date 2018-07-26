#ifndef JTL_MOVING_AVERAGE_H
#define JTL_MOVING_AVERAGE_H

#include <stdint.h>
#include <bx/bx.h>
#include <bx/math.h>

namespace jtl
{
template<typename T, uint32_t N>
class moving_average
{
public:
	moving_average();
	~moving_average();

	T insert(T v);
	T getAverage() const;
	void getBounds(T& minT, T& maxT);
	T getStdDev();

private:
	T m_Data[N];
	T m_Total;
	uint32_t m_Count;
	uint32_t m_InsertPos;
};

template<typename T, uint32_t N>
moving_average<T, N>::moving_average() : m_Total(0), m_Count(0), m_InsertPos(0)
{
	bx::memSet(m_Data, 0, sizeof(T) * N);
}

template<typename T, uint32_t N>
moving_average<T, N>::~moving_average()
{
}

template<typename T, uint32_t N>
T moving_average<T, N>::insert(T v)
{
	m_Total -= m_Data[m_InsertPos];
	m_Total += v;
	m_Data[m_InsertPos] = v;
	m_InsertPos = (m_InsertPos + 1) % N;

	m_Count++;
	if (m_Count > N) {
		m_Count = N;
	}

	return getAverage();
}

template<typename T, uint32_t N>
T moving_average<T, N>::getAverage() const
{
	return m_Total / m_Count;
}

template<typename T, uint32_t N>
void moving_average<T, N>::getBounds(T& minT, T& maxT)
{
	if (m_Count == 0) {
		minT = T(0);
		maxT = T(0);
	}

	minT = m_Data[0];
	maxT = m_Data[0];
	const uint32_t n = m_Count;
	for (uint32_t i = 1; i < n; ++i) {
		T v = m_Data[i];
		minT = v < minT ? v : minT;
		maxT = v > maxT ? v : maxT;
	}
}

template<typename T, uint32_t N>
T moving_average<T, N>::getStdDev()
{
	if (m_Count == 0) {
		return T(0);
	}

	const T avg = getAverage();
	T stdDev = T(0.0);
	const uint32_t n = m_Count;
	for (uint32_t i = 0; i < n; ++i) {
		T v = m_Data[i];
		T d = v - avg;

		stdDev += d * d;
	}

	return bx::sqrt(stdDev / m_Count);
}
}

#endif
