#pragma once

template <typename T, size_t Size>
class RingBuffer
{
public:
	void Write(const T* input, size_t count)
	{
		// TODO: Implement this
		if (count > m_Size)
		{
			std::cerr << "Input data exceeds buffer size." << std::endl;
		}

		for (size_t i = 0; i < count; ++i)
		{
			m_Data[m_Head] = input[i];
			m_Head = (m_Head + 1) % m_Size;

			// Overwrite the oldest element if the buffer is full
			if (m_Head == m_Tail)
			{
				m_Tail = (m_Tail + 1) % m_Size;
			}
		}
	}

	void Read(T* output, size_t count)
	{
		// TODO: Implement this
	}

private:
	T m_Data[Size];
	const size_t m_Size = Size;
	size_t m_Head = 0;
	size_t m_Tail = 0;
};