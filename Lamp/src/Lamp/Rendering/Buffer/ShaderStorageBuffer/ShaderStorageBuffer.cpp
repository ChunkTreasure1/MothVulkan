#include "lppch.h"
#include "ShaderStorageBuffer.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"
#include "Lamp/Log/Log.h"

#include "Lamp/Rendering/Shader/ShaderUtility.h"

namespace Lamp
{
	ShaderStorageBuffer::ShaderStorageBuffer(uint64_t size, bool indirect)
		: m_isIndirect(indirect)
	{
		Resize(size);
	}

	ShaderStorageBuffer::ShaderStorageBuffer(uint64_t elementSize, uint32_t elementCount, bool indirectBuffer)
		: m_isDynamic(true), m_isIndirect(indirectBuffer)
	{
		const uint64_t minSSBOAlignment = GraphicsContext::GetDevice()->GetPhysicalDevice()->GetCapabilities().minSSBOOffsetAlignment;
		uint64_t alignedSize = elementSize;

		if (minSSBOAlignment > 0)
		{
			alignedSize = Utility::GetAlignedSize(alignedSize, minSSBOAlignment);
		}

		m_size = alignedSize;
		m_totalSize = alignedSize * (uint64_t)elementCount;

		const VkDeviceSize bufferSize = m_totalSize;

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

	ShaderStorageBuffer::~ShaderStorageBuffer()
	{
		Release();
	}

	void ShaderStorageBuffer::Resize(uint64_t newSize)
	{
		LP_CORE_ASSERT(!m_isDynamic, "Resizing not properly implemented for dynamic SSBOs!");

		if (newSize > m_size)
		{
			Release();
			
			m_size = newSize;
			m_totalSize = newSize;

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

	const uint64_t ShaderStorageBuffer::GetOffsetSize() const
	{
		const uint64_t minSSBOAlignment = GraphicsContext::GetDevice()->GetPhysicalDevice()->GetCapabilities().minSSBOOffsetAlignment;
		uint64_t dynamicAlignment = m_size;

		if (minSSBOAlignment > 0)
		{
			dynamicAlignment = Utility::GetAlignedSize(dynamicAlignment, minSSBOAlignment);
		}

		return dynamicAlignment;
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

	Ref<ShaderStorageBuffer> ShaderStorageBuffer::Create(uint64_t elementSize, uint32_t elementCount, bool indirectBuffer)
	{
		return CreateRef<ShaderStorageBuffer>(elementSize, elementCount, indirectBuffer);
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