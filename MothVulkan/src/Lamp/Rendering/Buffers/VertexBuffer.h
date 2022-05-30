#pragma once

#include "Lamp/Core/Graphics/VulkanAllocator.h"
#include "Lamp/Rendering/Vertex.h"

#include <vector>

namespace Lamp
{
	class VertexBuffer
	{
	public:
		VertexBuffer(const std::vector<Vertex>& vertices, uint32_t size);
		VertexBuffer(uint32_t size);
		~VertexBuffer();

		void SetData(const void* data, uint32_t size);
		void Bind(VkCommandBuffer commandBuffer, uint32_t binding = 0) const;

		static Ref<VertexBuffer> Create(const std::vector<Vertex>& vertices, uint32_t size);
		static Ref<VertexBuffer> Create(uint32_t size);

	private:
		VkBuffer m_buffer = nullptr;
		VmaAllocation m_bufferAllocation = nullptr;
	};
}