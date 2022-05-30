#pragma once

#include "Lamp/Core/Base.h"

#include <vma/VulkanMemoryAllocator.h>

namespace Lamp
{
	class GraphicsDevice;
	class VulkanAllocator
	{
	public:
		VulkanAllocator() = default;
		VulkanAllocator(const std::string& tag);

		~VulkanAllocator();
		
		VmaAllocation AllocateBuffer(VkBufferCreateInfo bufferCreateInfo, VmaMemoryUsage memoryUsage, VkBuffer& outBuffer);
		VmaAllocation AllocateImage(VkImageCreateInfo bufferCreateInfo, VmaMemoryUsage memoryUsage, VkImage& outImage);
	
		void Free(VmaAllocation allocation);
		void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);
		void DestroyImage(VkImage image, VmaAllocation allocation);

		template<typename T>
		T* MapMemory(VmaAllocation allocation)
		{
			T* data = nullptr;
			vmaMapMemory(VulkanAllocator::GetAllocator(), allocation, (void**)&data);
			return data;
		}
		
		void UnmapMemory(VmaAllocation allocation);

		static void Initialize(Ref<GraphicsDevice> graphicsDevice);
		static void Shutdown();

		static VmaAllocator& GetAllocator();
	private:
		
	#ifdef LP_ENABLE_DEBUG_ALLOCATIONS
		uint64_t m_allocatedBytes = 0;
		uint64_t m_freedBytes = 0;
	#endif
		std::string m_tag;
	};
}