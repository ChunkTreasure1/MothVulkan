#include "lppch.h"
#include "GraphicsDevice.h"

#include "Lamp/Log/Log.h"
#include "Lamp/Core/Base.h"

namespace Lamp
{
	PhysicalGraphicsDevice::PhysicalGraphicsDevice(VkInstance instance)
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		
		if (deviceCount == 0)
		{
			throw std::runtime_error("Failed to find GPU with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	
		for (const auto& device : devices)
		{
			vkGetPhysicalDeviceProperties(device, &m_physicalDeviceProperties);
			if (m_physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				m_physicalDevice = device;
			}
		}

		m_capabilities.minUBOOffsetAlignment = m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

		if (!m_physicalDevice)
		{
			throw std::runtime_error("Failed to find a supported device!");
		}

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
		LP_CORE_ASSERT(queueFamilyCount > 0, "No queue families supported!");

		std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

		int32_t i = 0;
		bool foundQueues = false;
		for (const auto& prop : queueFamilyProperties)
		{
			if (prop.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				m_queueIndices.graphicsQueueIndex = i;
				m_queueIndices.presentQueueIndex = i;
			}
			
			if (prop.queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				m_queueIndices.computeQueueIndex = i;
			}

			if (m_queueIndices.IsComplete())
			{
				foundQueues = true;
				break;
			}

			i++;
		}

		LP_CORE_ASSERT(foundQueues, "No fitting queue found!");
	}

	PhysicalGraphicsDevice::~PhysicalGraphicsDevice()
	{}

	Ref<PhysicalGraphicsDevice> PhysicalGraphicsDevice::Create(VkInstance instance)
	{
		return CreateRef<PhysicalGraphicsDevice>(instance);
	}

	GraphicsDevice::GraphicsDevice(Ref<PhysicalGraphicsDevice> physicalDevice, VkPhysicalDeviceFeatures enabledFeatures)
		: m_physicalDevice(physicalDevice)
	{
		const PhysicalGraphicsDevice::QueueIndices& queueIndices = physicalDevice->GetQueueIndices();
		
		std::set<int32_t> uniqueQueues = { queueIndices.computeQueueIndex, queueIndices.graphicsQueueIndex, queueIndices.presentQueueIndex };
		float queuePriority = 1.f;

		std::vector<VkDeviceQueueCreateInfo> deviceQueueInfos;
		
		for (uint32_t queue : uniqueQueues)
		{
			VkDeviceQueueCreateInfo& queueInfo = deviceQueueInfos.emplace_back();
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.pQueuePriorities = &queuePriority;
			queueInfo.queueCount = 1;
			queueInfo.queueFamilyIndex = queue;
		}

		VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering{};
		dynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
		dynamicRendering.dynamicRendering = VK_TRUE;

		VkPhysicalDeviceVulkan11Features vulkan11Features{};
		vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
		vulkan11Features.shaderDrawParameters = VK_TRUE;
		vulkan11Features.pNext = &dynamicRendering;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pNext = &vulkan11Features;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueInfos.size());
		createInfo.pQueueCreateInfos = deviceQueueInfos.data();
		createInfo.pEnabledFeatures = &enabledFeatures;

		std::vector<const char*> enabledExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
		createInfo.ppEnabledExtensionNames = enabledExtensions.data();
	
		#ifdef LP_ENABLE_VALIDATION
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();
		#endif

		LP_VK_CHECK(vkCreateDevice(physicalDevice->GetHandle(), &createInfo, nullptr, &m_device));

		vkGetDeviceQueue(m_device, queueIndices.graphicsQueueIndex, 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_device, queueIndices.computeQueueIndex, 0, &m_computeQueue);
	
		// Create main thread (faster to use) command pool
		{
			VkCommandPoolCreateInfo commandPoolInfo{};
			commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolInfo.queueFamilyIndex = m_physicalDevice->GetQueueIndices().graphicsQueueIndex;
			commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		
			LP_VK_CHECK(vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &m_graphicsCommandPool));
		}
	}

	GraphicsDevice::~GraphicsDevice()
	{
		for (auto& it : m_commandPoolMap)
		{
			vkDestroyCommandPool(m_device, it.second, nullptr);
		}
		m_commandPoolMap.clear();

		vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
		vkDestroyDevice(m_device, nullptr);
	}

	VkCommandBuffer GraphicsDevice::GetThreadSafeCommandBuffer(bool begin)
	{
		VkCommandPool commandPool;
		VkCommandBuffer commandBuffer;

		VkCommandPoolCreateInfo commandPoolInfo{};
		commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolInfo.queueFamilyIndex = m_physicalDevice->GetQueueIndices().graphicsQueueIndex;
		commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		
		LP_VK_CHECK(vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &commandPool));

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandBufferCount = 1;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		LP_VK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer));
		m_commandPoolMap.emplace(commandBuffer, commandPool);

		if (begin) [[likely]]
		{
			VkCommandBufferBeginInfo cmdBufferBegin{};
			cmdBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufferBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			LP_VK_CHECK(vkBeginCommandBuffer(commandBuffer, &cmdBufferBegin));
		}

		return commandBuffer;
	}

	VkCommandBuffer GraphicsDevice::GetCommandBuffer(bool begin)
	{
		VkCommandBuffer commandBuffer;

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandBufferCount = 1;
		allocInfo.commandPool = m_graphicsCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		
		LP_VK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer));

		if (begin) [[likely]]
		{
			VkCommandBufferBeginInfo cmdBufferBegin{};
			cmdBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufferBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			LP_VK_CHECK(vkBeginCommandBuffer(commandBuffer, &cmdBufferBegin));
		}

		return commandBuffer;
	}

	VkCommandBuffer GraphicsDevice::CreateSecondaryCommandBuffer()
	{
		VkCommandBuffer commandBuffer;
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_graphicsCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		allocInfo.commandBufferCount = 1;

		LP_VK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer));
		return commandBuffer;
	}

	void GraphicsDevice::FlushThreadSafeCommandBuffer(VkCommandBuffer cmdBuffer)
	{
		FlushThreadSafeCommandBuffer(cmdBuffer, m_graphicsQueue);
	}

	void GraphicsDevice::FlushThreadSafeCommandBuffer(VkCommandBuffer cmdBuffer, VkQueue queue)
	{
		LP_CORE_ASSERT(cmdBuffer != VK_NULL_HANDLE, "Unable to flush null command buffer!");
		LP_CORE_ASSERT(m_commandPoolMap.find(cmdBuffer) != m_commandPoolMap.end(), "Command buffer not found in map! Was this command buffer created from the device?");
		
		const std::lock_guard lock(m_commandBufferFlushMutex);
		LP_VK_CHECK(vkEndCommandBuffer(cmdBuffer));

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = 0;

		VkFence fence;
		LP_VK_CHECK(vkCreateFence(m_device, &fenceInfo, nullptr, &fence));
		LP_VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, fence));
		LP_VK_CHECK(vkWaitForFences(m_device, 1, &fence, VK_TRUE, 1000000000));

		vkDestroyFence(m_device, fence, nullptr);

		VkCommandPool cmdPool = m_commandPoolMap.at(cmdBuffer);
		vkDestroyCommandPool(m_device, cmdPool, nullptr);

		m_commandPoolMap.erase(cmdBuffer);
	}

	void GraphicsDevice::FreeThreadSafeCommandBuffer(VkCommandBuffer cmdBuffer)
	{
		LP_CORE_ASSERT(m_commandPoolMap.find(cmdBuffer) != m_commandPoolMap.end(), "Command buffer not found in map! Was this command buffer created from the device?");
		VkCommandPool cmdPool = m_commandPoolMap.at(cmdBuffer);
		vkDestroyCommandPool(m_device, cmdPool, nullptr);

		m_commandPoolMap.erase(cmdBuffer);
	}

	void GraphicsDevice::FlushCommandBuffer(VkCommandBuffer cmdBuffer)
	{
		FlushCommandBuffer(cmdBuffer, m_graphicsQueue);
	}

	void GraphicsDevice::FlushCommandBuffer(VkCommandBuffer cmdBuffer, VkQueue queue)
	{
		LP_CORE_ASSERT(cmdBuffer != VK_NULL_HANDLE, "Unable to flush null command buffer!");
		LP_VK_CHECK(vkEndCommandBuffer(cmdBuffer));

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = 0;

		VkFence fence;
		LP_VK_CHECK(vkCreateFence(m_device, &fenceInfo, nullptr, &fence));
		LP_VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, fence));
		LP_VK_CHECK(vkWaitForFences(m_device, 1, &fence, VK_TRUE, 1000000000));

		vkDestroyFence(m_device, fence, nullptr);
		vkFreeCommandBuffers(m_device, m_graphicsCommandPool, 1, &cmdBuffer);
	}

	void GraphicsDevice::FreeCommandBuffer(VkCommandBuffer cmdBuffer)
	{
		vkFreeCommandBuffers(m_device, m_graphicsCommandPool, 1, &cmdBuffer);
	}

	Ref<GraphicsDevice> GraphicsDevice::Create(Ref<PhysicalGraphicsDevice> physicalDevice, VkPhysicalDeviceFeatures enabledFeatures)
	{
		return CreateRef<GraphicsDevice>(physicalDevice, enabledFeatures);
	}
}