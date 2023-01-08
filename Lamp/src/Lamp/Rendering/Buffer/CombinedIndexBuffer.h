#pragma once

#include "Lamp/Core/Graphics/VulkanAllocator.h"

namespace Lamp
{
	class CombinedIndexBuffer
	{
	public:
		CombinedIndexBuffer(uint32_t count);
		~CombinedIndexBuffer();

		void Bind(VkCommandBuffer commandBuffer) const;

		const uint32_t AppendToBuffer(uint32_t* indices, uint32_t count);
		static Ref<CombinedIndexBuffer> Create(uint32_t count);

	private:
		void Initialize();

		VkBuffer m_buffer = nullptr;
		VmaAllocation m_bufferAllocation = nullptr;

		uint64_t m_currentCount = 0;
		uint64_t m_totalSize = 0;
		uint64_t m_usedSize = 0;
	};
}