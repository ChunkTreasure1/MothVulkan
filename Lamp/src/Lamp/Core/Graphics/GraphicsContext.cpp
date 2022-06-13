#include "lppch.h"
#include "GraphicsContext.h"

#include "Lamp/Core/Base.h"
#include "Lamp/Log/Log.h"

#include "Lamp/Core/Graphics/GraphicsDevice.h"
#include "Lamp/Core/Graphics/VulkanAllocator.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace Lamp
{
	namespace Utility
	{
		static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
		{
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

			if (func != nullptr)
			{
				return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
			}
			else
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}

		static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
		{
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

			if (func != nullptr)
			{
				func(instance, debugMessenger, pAllocator);
			}
		}
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		switch (messageSeverity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				LP_CORE_TRACE("Validation layer: {0}", pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				LP_CORE_INFO("Validation layer: {0}", pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				LP_CORE_WARN("Validation layer: {0}", pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				LP_CORE_ERROR("Validation layer: {0}", pCallbackData->pMessage);
				break;
		}

		return VK_FALSE;
	}

	GraphicsContext::GraphicsContext()
	{
		LP_CORE_ASSERT(s_instance == nullptr, "Graphics context already exists!");
		s_instance = this;

		Initialize();
	}

	GraphicsContext::~GraphicsContext()
	{
		s_instance = nullptr;
		Shutdown();
	}

	void GraphicsContext::Initialize()
	{
		CreateVulkanInstance();
		SetupDebugCallback();

		m_physicalDevice = PhysicalGraphicsDevice::Create(m_vulkanInstance);

		VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering{};
		dynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
		dynamicRendering.dynamicRendering = VK_TRUE;

		VkPhysicalDeviceVulkan11Features vulkan11Features{};
		vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
		vulkan11Features.pNext = &dynamicRendering;		
		vulkan11Features.shaderDrawParameters = VK_TRUE;

		VkPhysicalDeviceVulkan12Features vk12Features{};
		vk12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		vk12Features.pNext = &vulkan11Features;
		vk12Features.drawIndirectCount = VK_TRUE;

		VkPhysicalDeviceFeatures2 enabledFeatures{};
		enabledFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		enabledFeatures.features.multiDrawIndirect = VK_TRUE;
		enabledFeatures.pNext = &vk12Features;

		m_device = GraphicsDevice::Create(m_physicalDevice, enabledFeatures);

		VulkanAllocator::Initialize(m_device);
	}

	void GraphicsContext::Shutdown()
	{
		VulkanAllocator::Shutdown();
		
		m_device = nullptr;
		m_physicalDevice = nullptr;

#ifdef LP_ENABLE_VALIDATION
		Utility::DestroyDebugUtilsMessengerEXT(m_vulkanInstance, m_debugMessenger, nullptr);
#endif

		vkDestroyInstance(m_vulkanInstance, nullptr);
	}

	GraphicsContext& GraphicsContext::Get()
	{
		return *s_instance;
	}

	Ref<GraphicsDevice> GraphicsContext::GetDevice()
	{
		return s_instance->m_device;
	}

	Ref<PhysicalGraphicsDevice> GraphicsContext::GetPhysicalDevice()
	{
		return s_instance->m_physicalDevice;
	}

	Ref<GraphicsContext> GraphicsContext::Create()
	{
		return CreateRef<GraphicsContext>();
	}

	void GraphicsContext::CreateVulkanInstance()
	{
		#ifdef LP_ENABLE_VALIDATION
		if (!CheckValidationLayerSupport())
		{
			LP_CORE_ERROR("Validation layers requested, but not available!");
			return;
		}
		#endif

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Lamp";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Lamp";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		auto extensions = GetRequiredExtensions();

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		#ifdef LP_ENABLE_VALIDATION
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		PopulateDebugMessengerCreateInfo(debugCreateInfo);

		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;

		createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();
		#else
		createInfo.pNext = nullptr;
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
		#endif

		LP_VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_vulkanInstance));
	}

	void GraphicsContext::SetupDebugCallback()
	{
		#ifdef LP_ENABLE_VALIDATION
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		PopulateDebugMessengerCreateInfo(createInfo);

		Utility::CreateDebugUtilsMessengerEXT(m_vulkanInstance, &createInfo, nullptr, &m_debugMessenger);
		#endif
	}

	bool GraphicsContext::CheckValidationLayerSupport()
	{
		uint32_t layerCount;
		LP_VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

		std::vector<VkLayerProperties> layerProperties(layerCount);
		LP_VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data()));

		for (const char* name : m_validationLayers)
		{
			bool layerFound = false;
			for (const auto& layerProp : layerProperties)
			{
				if (strcmp(name, layerProp.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}
		}

		return true;
	}
	void GraphicsContext::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& outCreateInfo)
	{
		outCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		outCreateInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

		outCreateInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		outCreateInfo.pfnUserCallback = VulkanDebugCallback;
		outCreateInfo.pUserData = nullptr;
	}

	std::vector<const char*> GraphicsContext::GetRequiredExtensions()
	{
		uint32_t extensionCount = 0;
		const char** extensions;

		extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
		std::vector<const char*> extensionsVector{ extensions, extensions + extensionCount };

		#ifdef LP_ENABLE_VALIDATION
		extensionsVector.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		#endif

		return extensionsVector;
	}
}