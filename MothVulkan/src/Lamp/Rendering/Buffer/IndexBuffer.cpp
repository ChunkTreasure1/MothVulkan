#include "lppch.h"
#include "IndexBuffer.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"

namespace Lamp
{
	IndexBuffer::IndexBuffer(const std::vector<uint32_t>& indices, uint32_t count)
		: m_count(count)
	{
		SetData(indices.data(), sizeof(uint32_t) * count);
	}

	IndexBuffer::IndexBuffer(uint32_t* indices, uint32_t count)
		: m_count(count)
	{
		SetData(indices, sizeof(uint32_t) * count);
	}

	IndexBuffer::~IndexBuffer()
	{
		if (m_buffer)
		{
			VulkanAllocator allocator;
			allocator.DestroyBuffer(m_buffer, m_bufferAllocation);
		}
	}

	void IndexBuffer::Bind(VkCommandBuffer commandBuffer)
	{
		const VkDeviceSize offset = 0;
		vkCmdBindIndexBuffer(commandBuffer, m_buffer, offset, VK_INDEX_TYPE_UINT32);
	}

	Ref<IndexBuffer> IndexBuffer::Create(const std::vector<uint32_t>& indices, uint32_t count)
	{
		return CreateRef<IndexBuffer>(indices, count);
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
	{
		return CreateRef<IndexBuffer>(indices, count);
	}

	void IndexBuffer::SetData(const void* data, uint32_t size)
	{
		auto device = GraphicsContext::GetDevice();

		VkBuffer stagingBuffer;
		VmaAllocation stagingAllocation;
		VkDeviceSize bufferSize = size;
		VulkanAllocator allocator{ "IndexBuffer" };

		if (m_buffer != VK_NULL_HANDLE)
		{
			allocator.DestroyBuffer(m_buffer, m_bufferAllocation);
		}

		if (data != nullptr)
		{
			// Create staging buffer
			{
				VkBufferCreateInfo bufferInfo{};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = bufferSize;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				stagingAllocation = allocator.AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer);
			}

			// Copy to staging buffer
			{
				void* buffData = allocator.MapMemory<void>(stagingAllocation);
				memcpy_s(buffData, size, data, size);
				allocator.UnmapMemory(stagingAllocation);
			}
		}

		// Create GPU buffer
		{
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = bufferSize;
			bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			m_bufferAllocation = allocator.AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_GPU_ONLY, m_buffer);
		}

		if (data != nullptr)
		{
			// Copy from staging buffer to GPU buffer
			{
				VkCommandBuffer cmdBuffer = device->GetThreadSafeCommandBuffer(true);

				VkBufferCopy copy{};
				copy.srcOffset = 0;
				copy.dstOffset = 0;
				copy.size = bufferSize;

				vkCmdCopyBuffer(cmdBuffer, stagingBuffer, m_buffer, 1, &copy);
				device->FlushThreadSafeCommandBuffer(cmdBuffer);
			}

			allocator.DestroyBuffer(stagingBuffer, stagingAllocation);
		}
	}
}