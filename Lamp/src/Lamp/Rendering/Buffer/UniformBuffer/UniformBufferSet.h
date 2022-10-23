#pragma once

#include "Lamp/Core/Base.h"

#include "Lamp/Rendering/Buffer/UniformBuffer/UniformBuffer.h"

#include <vector>

namespace Lamp
{
	class UniformBuffer;
	class UniformBufferSet
	{
	public:
		UniformBufferSet(const void* data, uint32_t size, uint32_t bufferCount);
		UniformBufferSet(uint32_t size, uint32_t bufferCount);
		UniformBufferSet(uint32_t sizePerObject, uint32_t objectCount, uint32_t bufferCount);
		~UniformBufferSet();

		inline const Ref<UniformBuffer> Get(uint32_t index) const { return m_uniformBuffers[index]; }

		static Ref<UniformBufferSet> Create(const void* data, uint32_t size, uint32_t bufferCount);
		static Ref<UniformBufferSet> Create(uint32_t size, uint32_t bufferCount);
		static Ref<UniformBufferSet> Create(uint32_t sizePerObject, uint32_t objectCount, uint32_t bufferCount);

	private:
		std::vector<Ref<UniformBuffer>> m_uniformBuffers;
	};
}