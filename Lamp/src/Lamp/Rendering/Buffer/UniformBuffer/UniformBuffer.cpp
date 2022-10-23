#include "lppch.h"
#include "UniformBuffer.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"
#include "Lamp/Log/Log.h"

#include "Lamp/Rendering/Shader/ShaderUtility.h"

namespace Lamp
{
	UniformBuffer::UniformBuffer(const void* data, uint32_t size)
		: m_size(size), m_totalSize(size)
	{
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

	UniformBuffer::UniformBuffer(uint32_t sizePerObject, uint32_t objectCount)
	{
		m_isDynamic = true;

		const uint64_t minUBOAlignment = GraphicsContext::GetDevice()->GetPhysicalDevice()->GetCapabilities().minUBOOffsetAlignment;
		uint32_t alignedSize = sizePerObject;

		if (minUBOAlignment > 0)
		{
			alignedSize = (uint32_t)Utility::GetAlignedSize((uint64_t)alignedSize, minUBOAlignment);
		}

		m_size = alignedSize;
		m_totalSize = alignedSize * objectCount;

		const VkDeviceSize bufferSize = m_totalSize;
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
	}

	UniformBuffer::~UniformBuffer()
	{
		VulkanAllocator allocator{ "UniformBuffer - Destroy" };
		allocator.DestroyBuffer(m_buffer, m_bufferAllocation);
	}

	void UniformBuffer::SetData(const void* data, uint32_t dataSize)
	{
		LP_CORE_ASSERT(m_size >= dataSize, "Unable to set data of larger size than buffer!");

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
	Ref<UniformBuffer> UniformBuffer::Create(uint32_t sizePerObject, uint32_t objectCount)
	{
		return CreateRef<UniformBuffer>(sizePerObject, objectCount);
	}
}