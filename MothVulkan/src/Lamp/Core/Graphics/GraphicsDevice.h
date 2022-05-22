#pragma once

#include "Lamp/Core/Base.h"

#include <vulkan/vulkan.h>

namespace Lamp
{
	class PhysicalGraphicsDevice
	{
	public:
		struct QueueIndices
		{
			bool IsComplete() const
			{
				return graphicsQueueIndex != -1 && presentQueueIndex != -1 && computeQueueIndex != -1;
			}

			int32_t graphicsQueueIndex = -1;
			int32_t presentQueueIndex = -1;
			int32_t computeQueueIndex = -1;
		};

		PhysicalGraphicsDevice(VkInstance instance);
		~PhysicalGraphicsDevice();

		inline VkPhysicalDevice GetHandle() const { return m_physicalDevice; }
		inline const QueueIndices& GetQueueIndices() const { return m_queueIndices; }

		static Ref<PhysicalGraphicsDevice> Create(VkInstance instance);

	private:
		QueueIndices m_queueIndices;

		VkPhysicalDevice m_physicalDevice = nullptr;
		VkPhysicalDeviceProperties m_physicalDeviceProperties;
	};

	class GraphicsDevice
	{
	public:
		GraphicsDevice(Ref<PhysicalGraphicsDevice> physicalDevice, VkPhysicalDeviceFeatures enabledFeatures);
		~GraphicsDevice();

		inline VkDevice GetHandle() const { return m_device; }
		inline VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
		inline VkQueue GetComputeQueue() const { return m_computeQueue; }

		static Ref<GraphicsDevice> Create(Ref<PhysicalGraphicsDevice> physicalDevice, VkPhysicalDeviceFeatures enabledFeatures);

	private:
		const std::vector<const char*> m_validationLayers = { "VK_LAYER_KHRONOS_validation" };
	
		VkDevice m_device;
		VkQueue m_graphicsQueue;
		VkQueue m_computeQueue;
	};
}