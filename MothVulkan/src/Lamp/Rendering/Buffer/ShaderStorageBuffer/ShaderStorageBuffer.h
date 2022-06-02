#pragma once

#include "Lamp/Core/Graphics/VulkanAllocator.h"

namespace Lamp
{
	class ShaderStorageBuffer
	{
	public:
		ShaderStorageBuffer(uint64_t size, bool indirectBuffer = false);
		~ShaderStorageBuffer();

		void Resize(uint64_t newSize);

		inline const VkBuffer GetHandle() const { return m_buffer; }
		inline const uint64_t GetSize() const { return m_size; }

		template<typename T>
		T* Map();
		void Unmap();

		static Ref<ShaderStorageBuffer> Create(uint64_t size, bool indirectBuffer = false);

	private:
		void Release();

		bool m_isIndirect = false;
		uint64_t m_size = 0;
		VkBuffer m_buffer = nullptr;
		VmaAllocation m_bufferAllocation = nullptr;
	};

	template<typename T>
	inline T* ShaderStorageBuffer::Map()
	{
		VulkanAllocator allocator{};
		return allocator.MapMemory<T>(m_bufferAllocation);
	}

}