#pragma once
#include <iostream>
template <typename T, size_t Size>
class RingBuffer
{
public:
	void Write(const T* input, size_t count)
	{
		// TODO: Implement this
		if (count > m_Size)
		{
			std::cerr << "Buffer size is not enough for input data!" << std::endl;
		}

		for (int curHead = 0; curHead != count; curHead++)
		{
			m_Data[m_Head] = input[curHead];
			m_Head = (m_Head + 1) % m_Size;

			if (m_Head == m_Tail)
			{
				m_Tail = (m_Tail + 1) % m_Size;
			}
		}
	}

	void Read(T* output, size_t count)
	{
		// TODO: Implement this
		int reqCount;
		if (m_Head >= m_Tail)
		{
			reqCount = m_Head - m_Tail;
		}
		else
		{
			reqCount = m_Size - m_Tail + m_Head;
		}

		if (count > reqCount)
		{
			std::cerr << "Data Read Error!" << std::endl;
			return;
		}
		else {
			for (int curHead = 0; curHead != count; curHead++)
			{
				output[curHead] = m_Data[m_Tail];
				m_Tail = (m_Tail + 1) % m_Size;
			}
		}
	}

	size_t GetSize() {
		return m_Size;
	}

	T* GetData(size_t& dataSize)
	{
		if (m_Count == 0)
		{
			// No data in the buffer
			dataSize = 0;
			return nullptr;
		}

		dataSize = m_Count;
		T* data = new T[dataSize];  // Allocate new memory to return the data


		if (m_Tail < m_Head)
		{
			std::memcpy(data, &m_Data[m_Tail], dataSize * sizeof(T));
		}
		else
		{
			size_t firstPartSize = m_Size - m_Tail;
			std::memcpy(data, &m_Data[m_Tail], firstPartSize * sizeof(T));
			std::memcpy(&data[firstPartSize], m_Data, (dataSize - firstPartSize) * sizeof(T));
		}

		return data;
	}

private:
	T m_Data[Size];
	const size_t m_Size = Size;
	size_t m_Head = 0;
	size_t m_Tail = 0;
	size_t m_Count = 0;
};