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

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
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
	}

	GraphicsDevice::~GraphicsDevice()
	{ 
		vkDestroyDevice(m_device, nullptr);
	}

	Ref<GraphicsDevice> GraphicsDevice::Create(Ref<PhysicalGraphicsDevice> physicalDevice, VkPhysicalDeviceFeatures enabledFeatures)
	{
		return CreateRef<GraphicsDevice>(physicalDevice, enabledFeatures);
	}
}