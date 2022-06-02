#include "lppch.h"
#include "ShaderStorageBuffer.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"

namespace Lamp
{
	ShaderStorageBuffer::ShaderStorageBuffer(uint64_t size, bool indirect)
		: m_isIndirect(indirect)
	{
		Resize(size);
	}

	ShaderStorageBuffer::~ShaderStorageBuffer()
	{
		Release();
	}

	void ShaderStorageBuffer::Resize(uint64_t newSize)
	{
		if (newSize > m_size)
		{
			Release();
			
			m_size = newSize;
			auto device = GraphicsContext::GetDevice();
			const VkDeviceSize bufferSize = newSize;

			VulkanAllocator allocator{ "ShaderStorageBuffer - Create" };

			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = bufferSize;
			bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			if (m_isIndirect)
			{
				bufferInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
				bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			}

			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			m_bufferAllocation = allocator.AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, m_buffer);
		}
	}

	void ShaderStorageBuffer::Unmap()
	{
		VulkanAllocator allocator{};
		allocator.UnmapMemory(m_bufferAllocation);
	}

	Ref<ShaderStorageBuffer> ShaderStorageBuffer::Create(uint64_t size, bool indirectBuffer)
	{
		return CreateRef<ShaderStorageBuffer>(size, indirectBuffer);
	}

	void ShaderStorageBuffer::Release()
	{
		if (m_buffer != VK_NULL_HANDLE)
		{
			VulkanAllocator allocator{ "ShaderStorageBuffer - Destroy" };
			allocator.DestroyBuffer(m_buffer, m_bufferAllocation);

			m_buffer = nullptr;
			m_bufferAllocation = nullptr;
		}
	}
}