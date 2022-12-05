#pragma once

#include "Lamp/Core/Graphics/VulkanAllocator.h"

namespace Lamp
{
	class ShaderStorageBuffer
	{
	public:
		ShaderStorageBuffer(uint64_t size, bool indirectBuffer = false);
		ShaderStorageBuffer(uint64_t elementSize, uint32_t elementCount, bool indirectBuffer = false);
		~ShaderStorageBuffer();

		void Resize(uint64_t newSize);

		inline const VkBuffer GetHandle() const { return m_buffer; }
		inline const uint64_t GetSize() const { return m_size; }
		inline const uint64_t GetTotalSize() const { return m_size; }

		const uint64_t GetOffsetSize() const;

		template<typename T>
		T* Map();
		void Unmap();

		static Ref<ShaderStorageBuffer> Create(uint64_t size, bool indirectBuffer = false);
		static Ref<ShaderStorageBuffer> Create(uint64_t elementSize, uint32_t elementCount, bool indirectBuffer = false);

	private:
		void Release();

		uint64_t m_size = 0;
		uint64_t m_totalSize = 0;

		bool m_isDynamic = false;
		bool m_isIndirect = false;

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