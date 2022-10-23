#include "lppch.h"
#include "CommandBuffer.h"

#include "Lamp/Core/Graphics/GraphicsDevice.h"
#include "Lamp/Core/Graphics/GraphicsContext.h"

#include "Lamp/Core/Application.h"
#include "Lamp/Core/Window.h"
#include "Lamp/Core/Graphics/Swapchain.h"

#include "Lamp/Log/Log.h"

namespace Lamp
{
	CommandBuffer::CommandBuffer(uint32_t count, bool swapchainTarget)
		: m_count(count), m_swapchainTarget(swapchainTarget)
	{
		auto device = GraphicsContext::GetDevice();
		if (!swapchainTarget)
		{
			m_commandPools.resize(count);
			m_commandBuffers.resize(count);
			m_submitFences.resize(count);

			for (uint32_t i = 0; i < count; i++)
			{
				VkCommandPoolCreateInfo poolInfo{};
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.queueFamilyIndex = GraphicsContext::GetPhysicalDevice()->GetQueueIndices().graphicsQueueIndex;
				poolInfo.flags = 0;

				LP_VK_CHECK(vkCreateCommandPool(device->GetHandle(), &poolInfo, nullptr, &m_commandPools[i]));
			}

			for (uint32_t i = 0; i < count; i++)
			{
				VkCommandBufferAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.commandPool = m_commandPools[i];
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandBufferCount = 1;

				LP_VK_CHECK(vkAllocateCommandBuffers(device->GetHandle(), &allocInfo, &m_commandBuffers[i]));
			}

			for (uint32_t i = 0; i < count; i++)
			{
				VkFenceCreateInfo fenceInfo{};
				fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
				LP_VK_CHECK(vkCreateFence(device->GetHandle(), &fenceInfo, nullptr, &m_submitFences[i]));
			}
		}
		else
		{
			const auto& swapchain = Application::Get().GetWindow()->GetSwapchain();
			m_count = swapchain.GetFramesInFlight();
			m_commandPools.resize(m_count);
			m_commandBuffers.resize(m_count);

			for (uint32_t i = 0; i < m_count; i++)
			{
				m_commandBuffers[i] = swapchain.GetCommandBuffer(i);
				m_commandPools[i] = swapchain.GetCommandPool(i);
			}
		}
	}

	CommandBuffer::~CommandBuffer()
	{
		if (!m_swapchainTarget)
		{
			auto device = GraphicsContext::GetDevice();

			for (auto& fence : m_submitFences)
			{
				vkDestroyFence(device->GetHandle(), fence, nullptr);
			}

			for (uint32_t i = 0; i < m_commandPools.size(); i++)
			{
				vkDestroyCommandPool(device->GetHandle(), m_commandPools[i], nullptr);
			}

			m_commandPools.clear();
			m_commandBuffers.clear();
		}
	}

	void CommandBuffer::Begin()
	{
		auto device = GraphicsContext::GetDevice();
		const uint32_t index = m_swapchainTarget ? Application::Get().GetWindow()->GetSwapchain().GetCurrentFrame() : m_currentCommandPool;

		if (!m_swapchainTarget)
		{
			vkWaitForFences(device->GetHandle(), 1, &m_submitFences[index], VK_TRUE, UINT64_MAX);
			LP_VK_CHECK(vkResetCommandPool(device->GetHandle(), m_commandPools[index], 0));
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		LP_VK_CHECK(vkBeginCommandBuffer(m_commandBuffers[index], &beginInfo));
	
		OPTICK_GPU_CONTEXT(m_commandBuffers[index]);
		OPTICK_GPU_EVENT("Test");
	}

	void CommandBuffer::End()
	{
		auto device = GraphicsContext::GetDevice();
		const uint32_t index = m_swapchainTarget ? Application::Get().GetWindow()->GetSwapchain().GetCurrentFrame() : m_currentCommandPool;

		LP_VK_CHECK(vkEndCommandBuffer(m_commandBuffers[index]));

		if (!m_swapchainTarget)
		{
			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_commandBuffers[index];

			vkResetFences(device->GetHandle(), 1, &m_submitFences[index]);
			LP_VK_CHECK(vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo, m_submitFences[index]));
		}

		m_currentCommandPool = (m_currentCommandPool + 1) % m_count;
	}

	VkCommandBuffer CommandBuffer::GetCurrentCommandBuffer()
	{
		const uint32_t index = m_swapchainTarget ? Application::Get().GetWindow()->GetSwapchain().GetCurrentFrame() : m_currentCommandPool;
		return m_commandBuffers[index];
	}

	uint32_t CommandBuffer::GetCurrentIndex()
	{
		const uint32_t index = m_swapchainTarget ? Application::Get().GetWindow()->GetSwapchain().GetCurrentFrame() : m_currentCommandPool;
		return index;
	}

	Ref<CommandBuffer> CommandBuffer::Create(uint32_t count, bool swapchainTarget)
	{
		return CreateRef<CommandBuffer>(count, swapchainTarget);
	}

}