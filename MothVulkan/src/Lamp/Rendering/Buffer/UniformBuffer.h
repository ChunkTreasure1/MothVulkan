#pragma once

#include "Lamp/Core/Graphics/VulkanAllocator.h"

namespace Lamp
{
	class UniformBuffer
	{
	public:
		UniformBuffer(const void* data, uint32_t size);
		~UniformBuffer();

		inline const VkBuffer GetHandle() const { return m_buffer; }

		void SetData(const void* data, uint32_t size);
		
		template<typename T>
		T* Map();
		void Unmap();

		static Ref<UniformBuffer> Create(const void* data, uint32_t size);

	private:
		uint32_t m_size;
		VkBuffer m_buffer = nullptr;
		VmaAllocation m_bufferAllocation = nullptr;
	};
	
	template<typename T>
	inline T* UniformBuffer::Map()
	{
		VulkanAllocator allocator{};
		return allocator.MapMemory<T>(m_bufferAllocation);
	}

}