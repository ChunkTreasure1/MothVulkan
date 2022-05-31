#pragma once

#include "Lamp/Core/Graphics/VulkanAllocator.h"

#include <vector>

namespace Lamp
{
	class IndexBuffer
	{
	public:
		IndexBuffer(const std::vector<uint32_t>& indices, uint32_t count);
		IndexBuffer(uint32_t* indices, uint32_t count);
		~IndexBuffer();

		void Bind(VkCommandBuffer commandBuffer);

		static Ref<IndexBuffer> Create(const std::vector<uint32_t>& pIndices, uint32_t count);
		static Ref<IndexBuffer> Create(uint32_t* pIndices, uint32_t count);

	private:
		void SetData(const void* data, uint32_t size);

		VkBuffer m_buffer = nullptr;
		VmaAllocation m_bufferAllocation = nullptr;
		uint32_t m_count = 0;
	};
}