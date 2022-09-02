#pragma once

#include "Lamp/Core/Graphics/VulkanAllocator.h"

namespace Lamp
{
	class UniformBuffer
	{
	public:
		UniformBuffer(const void* data, uint32_t size);
		UniformBuffer(uint32_t sizePerObject, uint32_t objectCount);
		~UniformBuffer();

		inline const VkBuffer GetHandle() const { return m_buffer; }
		inline const uint32_t GetSize() const { return m_size; }
		inline const uint32_t GetTotalSize() const { return m_totalSize; }

		inline const bool IsDynamic() const { return m_isDynamic; }

		void SetData(const void* data, uint32_t size);
		
		template<typename T>
		T* Map();
		void Unmap();

		static Ref<UniformBuffer> Create(const void* data, uint32_t size);
		static Ref<UniformBuffer> Create(uint32_t sizePerObject, uint32_t objectCount);

	private:
		uint32_t m_size{};
		uint32_t m_totalSize{};
		bool m_isDynamic = false;

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