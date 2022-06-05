#pragma once

#include "Lamp/Core/Base.h"

#include <vulkan/vulkan.h>

namespace Lamp
{
	class CommandBuffer
	{
	public:
		CommandBuffer(uint32_t count, bool swapchainTarget);
		~CommandBuffer();
		
		void Begin();
		void End();

		VkCommandBuffer GetCurrentCommandBuffer();
		uint32_t GetCurrentIndex();
		
		static Ref<CommandBuffer> Create(uint32_t count, bool swapchainTarget = false);

	private:
		std::vector<VkCommandPool> m_commandPools;
		std::vector<VkCommandBuffer> m_commandBuffers;
		std::vector<VkFence> m_submitFences;

		bool m_swapchainTarget = false;
		uint32_t m_currentCommandPool = 0;
		uint32_t m_count = 0;
	};
}