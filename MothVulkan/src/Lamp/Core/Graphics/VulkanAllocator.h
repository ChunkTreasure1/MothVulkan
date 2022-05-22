#pragma once

#include <vma/VulkanMemoryAllocator.h>

namespace Lamp
{
	class VulkanAllocator
	{
		VulkanAllocator() = default;
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

		static void Initialize();
		static void Shutdown();

		//static VmaAllocator& GetAllocator();
	};
}