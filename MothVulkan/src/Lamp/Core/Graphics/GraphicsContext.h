#pragma once

#include "Lamp/Core/Base.h"

#include <vulkan/vulkan.h>

namespace Lamp
{
	class PhysicalGraphicsDevice;
	class GraphicsDevice;
	class GraphicsContext
	{
	public:
		GraphicsContext();
		~GraphicsContext();

		void Initialize();
		void Shutdown();

		inline const VkInstance GetInstance() const { return m_vulkanInstance; }

		static GraphicsContext& Get();
		static Ref<GraphicsDevice> GetDevice();
		static Ref<PhysicalGraphicsDevice> GetPhysicalDevice();

		static Ref<GraphicsContext> Create();

	private:
		void CreateVulkanInstance();
		void SetupDebugCallback();

		bool CheckValidationLayerSupport();
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& outCreateInfo);
		std::vector<const char*> GetRequiredExtensions();

		const std::vector<const char*> m_validationLayers = { "VK_LAYER_KHRONOS_validation" };

		Ref<PhysicalGraphicsDevice> m_physicalDevice;
		Ref<GraphicsDevice> m_device;

		VkInstance m_vulkanInstance;
		VkDebugUtilsMessengerEXT m_debugMessenger;

		inline static GraphicsContext* s_instance = nullptr;
	};
}