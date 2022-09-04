#include "lppch.h"
#include "Buffer.h"

#include "Lamp/Core/Base.h"
#include "Lamp/Log/Log.h"

namespace Lamp
{
	Buffer::Buffer(uint64_t size)
		: m_size(size)
	{
		Allocate(size);
	}

	Buffer::~Buffer()
	{
		if (m_data)
		{
			free(m_data);
			m_data = nullptr;
		}
	}
	
	bool Buffer::IsValid() const
	{
		return m_data != nullptr;
	}

	void Buffer::Allocate(uint64_t size)
	{
		Free();

		m_data = malloc(size);
		m_size = size;
	}

	void Buffer::Free()
	{
		if (m_data)
		{
			free(m_data);
			m_data = nullptr;
		}
	}

	void Buffer::Copy(const void* src, uint64_t size)
	{
		LP_CORE_ASSERT(size <= m_size, "Destination size is smaller than source size!");

		Allocate(size);
		memcpy_s(m_data, m_size, src, size);
	}
}