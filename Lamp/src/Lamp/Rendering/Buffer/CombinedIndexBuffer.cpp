#include "lppch.h"
#include "CombinedIndexBuffer.h"

#include "Lamp/Core/Graphics/GraphicsDevice.h"
#include "Lamp/Core/Graphics/GraphicsContext.h"

namespace Lamp
{
	CombinedIndexBuffer::CombinedIndexBuffer(uint32_t count)
		: m_totalSize(sizeof(uint32_t) * count)
	{
		Initialize();
	}

	CombinedIndexBuffer::~CombinedIndexBuffer()
	{
		if (m_buffer)
		{
			VulkanAllocator allocator{ "CombinedIndexBuffer - Destroy" };
			allocator.DestroyBuffer(m_buffer, m_bufferAllocation);
		}
	}

	void CombinedIndexBuffer::Bind(VkCommandBuffer commandBuffer) const
	{
		LP_PROFILE_FUNCTION();
		vkCmdBindIndexBuffer(commandBuffer, m_buffer, 0, VK_INDEX_TYPE_UINT32);
	}

	const uint32_t CombinedIndexBuffer::AppendToBuffer(uint32_t* indices, uint32_t count)
	{
		LP_CORE_ASSERT(m_totalSize > m_usedSize + sizeof(uint32_t) * count, "The buffer is too small to have more data!");

		const uint32_t location = (uint32_t)m_currentCount;
		const uint64_t size = sizeof(uint32_t) * count;

		VkBuffer stagingBuffer;
		VmaAllocation stagingAllocation;
		VulkanAllocator allocator{};

		// Create staging buffer
		{
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = size;
			bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			stagingAllocation = allocator.AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer);
		}

		// Copy to staging buffer
		{
			void* buffData = allocator.MapMemory<void>(stagingAllocation);
			memcpy_s(buffData, m_totalSize - m_usedSize, indices, size);
			allocator.UnmapMemory(stagingAllocation);
		}

		// Copy from staging buffer to GPU buffer
		{
			auto device = GraphicsContext::GetDevice();
			VkCommandBuffer cmdBuffer = device->GetThreadSafeCommandBuffer(true);

			VkBufferCopy copy{};
			copy.srcOffset = 0;
			copy.dstOffset = m_usedSize;
			copy.size = size;

			vkCmdCopyBuffer(cmdBuffer, stagingBuffer, m_buffer, 1, &copy);
			device->FlushThreadSafeCommandBuffer(cmdBuffer);
		}

		allocator.DestroyBuffer(stagingBuffer, stagingAllocation);
		m_usedSize += size;
		m_currentCount += count;

		return location;
	}

	Ref<CombinedIndexBuffer> CombinedIndexBuffer::Create(uint32_t count)
	{
		return CreateRef<CombinedIndexBuffer>(count);
	}

	void CombinedIndexBuffer::Initialize()
	{
		auto device = GraphicsContext::GetDevice();
		VulkanAllocator allocator{ "CombinedIndexBuffer - Create" };

		// Create GPU buffer
		{
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = m_totalSize;
			bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			m_bufferAllocation = allocator.AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, m_buffer);
		}
	}
}