#include "lppch.h"
#include "CombinedVertexBuffer.h"

#include "Lamp/Core/Graphics/GraphicsDevice.h"
#include "Lamp/Core/Graphics/GraphicsContext.h"

namespace Lamp
{
	CombinedVertexBuffer::CombinedVertexBuffer(uint64_t vertexSize, uint64_t count)
		: m_totalSize(vertexSize * count), m_vertexSize(vertexSize)
	{
		Initialize();
	}

	CombinedVertexBuffer::~CombinedVertexBuffer()
	{
		if (m_buffer != VK_NULL_HANDLE)
		{
			VulkanAllocator allocator{ "CombinedVertexBuffer - Destroy" };
			allocator.DestroyBuffer(m_buffer, m_bufferAllocation);
		}
	}

	void CombinedVertexBuffer::Bind(VkCommandBuffer commandBuffer, uint32_t binding) const
	{
		LP_PROFILE_FUNCTION();

		const VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, binding, 1, &m_buffer, &offset);
	}

	const uint64_t CombinedVertexBuffer::AppendToBuffer(const void* data, uint64_t count)
	{
		LP_CORE_ASSERT(m_totalSize > m_usedSize + m_vertexSize * count, "The buffer is too small to have more data!");

		const uint64_t location = m_currentCount;
		const uint64_t size = m_vertexSize * count;

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
			memcpy_s(buffData, m_totalSize - m_usedSize, data, size);
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

	Ref<CombinedVertexBuffer> CombinedVertexBuffer::Create(uint64_t vertexSize, uint64_t count)
	{
		return CreateRef<CombinedVertexBuffer>(vertexSize, count);
	}

	void CombinedVertexBuffer::Initialize()
	{
		auto device = GraphicsContext::GetDevice();
		VulkanAllocator allocator{ "VertexBuffer - Create" };
		
		// Create GPU buffer
		{
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = m_totalSize;
			bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			m_bufferAllocation = allocator.AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, m_buffer);
		}
	}
}