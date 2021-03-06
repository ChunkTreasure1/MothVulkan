#include "lppch.h"
#include "UniformBuffer.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Log/Log.h"

namespace Lamp
{
	UniformBuffer::UniformBuffer(const void* data, uint32_t size)
		: m_size(size)
	{
		auto device = GraphicsContext::GetDevice();
		const VkDeviceSize bufferSize = size;
		VulkanAllocator allocator{ "UniformBuffer - Create" };

		// Create buffer
		{
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = bufferSize;
			bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			m_bufferAllocation = allocator.AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, m_buffer);
		}

		if (data)
		{
			SetData(data, size);
		}
	}

	UniformBuffer::~UniformBuffer()
	{
		VulkanAllocator allocator{ "UniformBuffer - Destroy" };
		allocator.DestroyBuffer(m_buffer, m_bufferAllocation);
	}

	void UniformBuffer::SetData(const void* data, uint32_t dataSize)
	{
		LP_CORE_ASSERT(m_size >= dataSize, "Unable to set data of larger size than buffer!");

		auto device = GraphicsContext::GetDevice();
		VkDeviceSize bufferSize = dataSize;

		VulkanAllocator allocator{ "UniformBuffer - SetData" };

		void* bufferData = allocator.MapMemory<void*>(m_bufferAllocation);
		memcpy_s(bufferData, m_size, data, dataSize);
		allocator.UnmapMemory(m_bufferAllocation);
	}

	void UniformBuffer::Unmap()
	{
		VulkanAllocator allocator{};
		allocator.UnmapMemory(m_bufferAllocation);
	}

	Ref<UniformBuffer> UniformBuffer::Create(const void* data, uint32_t size)
	{
		return CreateRef<UniformBuffer>(data, size);
	}
}