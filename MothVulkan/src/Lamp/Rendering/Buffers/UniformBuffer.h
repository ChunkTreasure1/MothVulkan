#pragma once

#include "Lamp/Core/Graphics/VulkanAllocator.h"

namespace Lamp
{
	class UniformBuffer
	{
	public:
		UniformBuffer(const void* data, uint32_t size);
		~UniformBuffer();

		void SetData(const void* data, uint32_t size);
		
		static Ref<UniformBuffer> Create(const void* data, uint32_t size);

	private:
		uint32_t m_size;
		VkBuffer m_buffer = nullptr;
		VmaAllocation m_bufferAllocation = nullptr;
	};
}