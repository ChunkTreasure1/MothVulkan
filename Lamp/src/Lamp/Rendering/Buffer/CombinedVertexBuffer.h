#pragma once

#include "Lamp/Core/Graphics/VulkanAllocator.h"
#include "Lamp/Core/Base.h"

#include <cstdint>

namespace Lamp
{
	class CombinedVertexBuffer
	{
	public:
		CombinedVertexBuffer(uint64_t vertexSize, uint64_t count);
		~CombinedVertexBuffer();

		void Bind(VkCommandBuffer commandBuffer, uint32_t binding = 0) const;

		const uint64_t AppendToBuffer(const void* data, uint64_t count);
		static Ref<CombinedVertexBuffer> Create(uint64_t vertexSize, uint64_t size);

	private:
		void Initialize();

		VkBuffer m_buffer = nullptr;
		VmaAllocation m_bufferAllocation = nullptr;

		const uint64_t m_vertexSize;

		uint64_t m_currentCount = 0;
		uint64_t m_totalSize = 0;
		uint64_t m_usedSize = 0;
	};
}