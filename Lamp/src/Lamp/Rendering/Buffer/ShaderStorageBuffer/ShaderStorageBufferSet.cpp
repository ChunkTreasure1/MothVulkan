#include "lppch.h"
#include "ShaderStorageBufferSet.h"

namespace Lamp
{
	ShaderStorageBufferSet::ShaderStorageBufferSet(uint64_t size, uint32_t count, bool indirectBuffer)
	{
		m_storageBuffers.reserve(count);
		for (uint32_t i = 0; i < count; i++)
		{
			m_storageBuffers.emplace_back(ShaderStorageBuffer::Create(size, indirectBuffer));
		}
	}

	ShaderStorageBufferSet::~ShaderStorageBufferSet()
	{
		m_storageBuffers.clear();
	}

	Ref<ShaderStorageBufferSet> ShaderStorageBufferSet::Create(uint64_t size, uint32_t count, bool indirectBuffer)
	{
		return CreateRef<ShaderStorageBufferSet>(size, count, indirectBuffer);
	}
}