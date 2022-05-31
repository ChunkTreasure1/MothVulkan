#include "lppch.h"
#include "UniformBufferSet.h"

#include "Lamp/Rendering/Buffer/UniformBuffer.h"

namespace Lamp
{
	UniformBufferSet::UniformBufferSet(const void* data, uint32_t size, uint32_t bufferCount)
	{
		m_uniformBuffers.reserve(bufferCount);
		for (uint32_t i = 0; i < bufferCount; i++)
		{
			m_uniformBuffers.emplace_back(UniformBuffer::Create(data, size));
		}
	}

	UniformBufferSet::UniformBufferSet(uint32_t size, uint32_t bufferCount)
	{
		m_uniformBuffers.reserve(bufferCount);
		for (uint32_t i = 0; i < bufferCount; i++)
		{
			m_uniformBuffers.emplace_back(UniformBuffer::Create(nullptr, size));
		}
	}

	UniformBufferSet::~UniformBufferSet()
	{
		m_uniformBuffers.clear();
	}

	Ref<UniformBufferSet> UniformBufferSet::Create(const void* data, uint32_t size, uint32_t bufferCount)
	{
		return CreateRef<UniformBufferSet>(data, size, bufferCount);
	}
	
	Ref<UniformBufferSet> UniformBufferSet::Create(uint32_t size, uint32_t bufferCount)
	{
		return CreateRef<UniformBufferSet>(size, bufferCount);
	}
}