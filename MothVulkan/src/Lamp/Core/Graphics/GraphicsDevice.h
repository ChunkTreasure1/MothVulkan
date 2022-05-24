#pragma once

#include "Lamp/Core/Base.h"

#include <vulkan/vulkan.h>

#include <mutex>

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
		~GraphicsDevice() = default;

		VkCommandBuffer GetThreadSafeCommandBuffer(bool begin);
		VkCommandBuffer GetCommandBuffer(bool begin);

		void FlushThreadSafeCommandBuffer(VkCommandBuffer cmdBuffer);
		void FlushThreadSafeCommandBuffer(VkCommandBuffer cmdBuffer, VkQueue queue);
		void FreeThreadSafeCommandBuffer(VkCommandBuffer cmdBuffer);
		
		void FlushCommandBuffer(VkCommandBuffer cmdBuffer);
		void FlushCommandBuffer(VkCommandBuffer cmdBuffer, VkQueue queue);
		void FreeCommandBuffer(VkCommandBuffer cmdBuffer);

		inline VkDevice GetHandle() const { return m_device; }
		inline VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
		inline VkQueue GetComputeQueue() const { return m_computeQueue; }
		
		inline Ref<PhysicalGraphicsDevice> GetPhysicalDevice() const { return m_physicalDevice; }
		static Ref<GraphicsDevice> Create(Ref<PhysicalGraphicsDevice> physicalDevice, VkPhysicalDeviceFeatures enabledFeatures);

	private:
		const std::vector<const char*> m_validationLayers = { "VK_LAYER_KHRONOS_validation" };
		std::mutex m_commandBufferFlushMutex;
		Ref<PhysicalGraphicsDevice> m_physicalDevice;
	
		std::unordered_map<VkCommandBuffer, VkCommandPool> m_commandPoolMap;
		VkCommandPool m_graphicsCommandPool;

		VkDevice m_device;
		VkQueue m_graphicsQueue;
		VkQueue m_computeQueue;
	};
}