#include "lppch.h"
#include "VertexBuffer.h"

#include "Lamp/Core/Graphics/GraphicsDevice.h"
#include "Lamp/Core/Graphics/GraphicsContext.h"

namespace Lamp
{
	VertexBuffer::VertexBuffer(const std::vector<Vertex>& vertices, uint32_t size)
	{
		SetData(vertices.data(), size);
	}

	VertexBuffer::VertexBuffer(uint32_t size)
	{
		SetData(nullptr, size);
	}

	VertexBuffer::~VertexBuffer()
	{
		if (m_buffer != VK_NULL_HANDLE)
		{
			VulkanAllocator allocator;
			allocator.DestroyBuffer(m_buffer, m_bufferAllocation);
		}
	}

	void VertexBuffer::SetData(const void* data, uint32_t size)
	{
		auto device = GraphicsContext::GetDevice();

		VkBuffer stagingBuffer;
		VmaAllocation stagingAllocation;
		VkDeviceSize bufferSize = size;
		VulkanAllocator allocator{ "VertexBuffer" };

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
			bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
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

	void VertexBuffer::Bind(VkCommandBuffer commandBuffer, uint32_t binding) const
	{
		const VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, binding, 1, &m_buffer, &offset);
	}

	Ref<VertexBuffer> VertexBuffer::Create(const std::vector<Vertex>& vertices, uint32_t size)
	{
		return CreateRef<VertexBuffer>(vertices, size);
	}

	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size)
	{
		return CreateRef<VertexBuffer>(size);
	}

}