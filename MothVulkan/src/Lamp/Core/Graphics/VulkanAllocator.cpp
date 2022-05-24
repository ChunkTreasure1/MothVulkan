#include "lppch.h"
#include "VulkanAllocator.h"

#include "Lamp/Core/Graphics/GraphicsDevice.h"
#include "Lamp/Core/Graphics/GraphicsContext.h"

#include "Lamp/Log/Log.h"

namespace Lamp
{
	struct VulkanAllocatorData
	{
		VmaAllocator allocator;
		uint64_t totalFreedBytes = 0;
		uint64_t totalAllocatedBytes = 0;
	};

	static VulkanAllocatorData* s_allocatorData = nullptr;

	VulkanAllocator::VulkanAllocator(const std::string& tag)
		: m_tag(tag)
	{}

	VulkanAllocator::~VulkanAllocator()
	{
	#ifdef LP_ENABLE_DEBUG_ALLOCATIONS
		if (m_tag.empty()) [[likely]]
		{
			LP_CORE_INFO("Anonymous VulkanAllocator allocated {0} bytes!", m_allocatedBytes);
		}
		else
		{
			LP_CORE_INFO("VulkanAllocator {0} allocated {1} bytes!", m_tag.c_str(), m_allocatedBytes);
		}
	#endif
	}

	VmaAllocation VulkanAllocator::AllocateBuffer(VkBufferCreateInfo bufferCreateInfo, VmaMemoryUsage memoryUsage, VkBuffer& outBuffer)
	{
		VmaAllocationCreateInfo allocCreateInfo{};
		allocCreateInfo.usage = memoryUsage;

		VmaAllocation allocation;
		vmaCreateBuffer(s_allocatorData->allocator, &bufferCreateInfo, &allocCreateInfo, &outBuffer, &allocation, nullptr);

		VmaAllocationInfo allocInfo{};
		vmaGetAllocationInfo(s_allocatorData->allocator, allocation, &allocInfo);

		s_allocatorData->totalAllocatedBytes += allocInfo.size;
		
	#ifdef LP_ENABLE_DEBUG_ALLOCATIONS
		m_allocatedBytes += (uint64_t)allocInfo.size;
	#endif
		
		return allocation;
	}

	VmaAllocation VulkanAllocator::AllocateImage(VkImageCreateInfo bufferCreateInfo, VmaMemoryUsage memoryUsage, VkImage& outImage)
	{
		VmaAllocationCreateInfo allocCreateInfo{};
		allocCreateInfo.usage = memoryUsage;
		
		VmaAllocation allocation;
		vmaCreateImage(s_allocatorData->allocator, &bufferCreateInfo, &allocCreateInfo, &outImage, &allocation, nullptr);
		
		VmaAllocationInfo allocInfo{};
		vmaGetAllocationInfo(s_allocatorData->allocator, allocation, &allocInfo);

		s_allocatorData->totalAllocatedBytes += allocInfo.size;
		
	#ifdef LP_ENABLE_DEBUG_ALLOCATIONS
		m_allocatedBytes += (uint64_t)allocInfo.size;
	#endif

		return allocation;
	}

	void VulkanAllocator::Free(VmaAllocation allocation)
	{
		LP_CORE_ASSERT(allocation, "Unable to free null allocation!");

		VmaAllocationInfo allocInfo{};
		vmaGetAllocationInfo(s_allocatorData->allocator, allocation, &allocInfo);
		s_allocatorData->totalFreedBytes += allocInfo.size;
		
		vmaFreeMemory(s_allocatorData->allocator, allocation);
	}

	void VulkanAllocator::DestroyBuffer(VkBuffer buffer, VmaAllocation allocation)
	{
		LP_CORE_ASSERT(buffer, "Unable to destroy null buffer!");
		LP_CORE_ASSERT(allocation, "Unable to free null allocation!");
	
		VmaAllocationInfo allocInfo{};
		vmaGetAllocationInfo(s_allocatorData->allocator, allocation, &allocInfo);
		s_allocatorData->totalFreedBytes += allocInfo.size;
	
		vmaDestroyBuffer(s_allocatorData->allocator, buffer, allocation);
	}

	void VulkanAllocator::DestroyImage(VkImage image, VmaAllocation allocation)
	{
		LP_CORE_ASSERT(image, "Unable to destroy null image!");
		LP_CORE_ASSERT(allocation, "Unable to free null allocation!");

		VmaAllocationInfo allocInfo{};
		vmaGetAllocationInfo(s_allocatorData->allocator, allocation, &allocInfo);
		s_allocatorData->totalFreedBytes += allocInfo.size;
	
		vmaDestroyImage(s_allocatorData->allocator, image, allocation);
	}

	void VulkanAllocator::UnmapMemory(VmaAllocation allocation)
	{
		vmaUnmapMemory(s_allocatorData->allocator, allocation);
	}

	void VulkanAllocator::Initialize(Ref<GraphicsDevice> graphicsDevice)
	{
		s_allocatorData = new VulkanAllocatorData();

		VmaAllocatorCreateInfo info{};
		info.vulkanApiVersion = VK_API_VERSION_1_3;
		info.physicalDevice = graphicsDevice->GetPhysicalDevice()->GetHandle();
		info.device = graphicsDevice->GetHandle();
		info.instance = GraphicsContext::Get().GetInstance();

		LP_VK_CHECK(vmaCreateAllocator(&info, &s_allocatorData->allocator));
	}

	void VulkanAllocator::Shutdown()
	{
		LP_CORE_ASSERT(s_allocatorData->totalAllocatedBytes == s_allocatorData->totalFreedBytes, "Some data has not been freed! This will cause a memory leak!");
		LP_CORE_ASSERT(s_allocatorData->allocator, "Unable to delete allocator as it does not exist!");

		vmaDestroyAllocator(s_allocatorData->allocator);
		s_allocatorData->allocator = nullptr;

		LP_CORE_ASSERT(s_allocatorData, "Unable to delete allocator data as it does not exist!");

		delete s_allocatorData;
		s_allocatorData = nullptr;
	}

	VmaAllocator& VulkanAllocator::GetAllocator()
	{
		return s_allocatorData->allocator;
	}
}