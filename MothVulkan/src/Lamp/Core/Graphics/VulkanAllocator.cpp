#include "lppch.h"
#include "VulkanAllocator.h"

namespace Lamp
{
	VulkanAllocator::~VulkanAllocator()
	{
	}

	VmaAllocation VulkanAllocator::AllocateBuffer(VkBufferCreateInfo bufferCreateInfo, VmaMemoryUsage memoryUsage, VkBuffer& outBuffer)
	{
		return VmaAllocation();
	}

	VmaAllocation VulkanAllocator::AllocateImage(VkImageCreateInfo bufferCreateInfo, VmaMemoryUsage memoryUsage, VkImage& outImage)
	{
		return VmaAllocation();
	}

	void VulkanAllocator::Free(VmaAllocation allocation)
	{
	}

	void VulkanAllocator::DestroyBuffer(VkBuffer buffer, VmaAllocation allocation)
	{
	}

	void VulkanAllocator::DestroyImage(VkImage image, VmaAllocation allocation)
	{
	}

	void VulkanAllocator::UnmapMemory(VmaAllocation allocation)
	{
	}

	void VulkanAllocator::Initialize()
	{
	}

	void VulkanAllocator::Shutdown()
	{
	}

}