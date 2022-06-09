#include "lppch.h"
#include "Swapchain.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"

#include "Lamp/Log/Log.h"

#include <GLFW/glfw3.h>

namespace Lamp
{
	Swapchain::Swapchain(GLFWwindow* window)
	{
		LP_VK_CHECK(glfwCreateWindowSurface(GraphicsContext::Get().GetInstance(), window, nullptr, &m_surface));
		const auto& queueIndices = GraphicsContext::GetPhysicalDevice()->GetQueueIndices();

		VkBool32 supportsPresentation;
		vkGetPhysicalDeviceSurfaceSupportKHR(GraphicsContext::GetPhysicalDevice()->GetHandle(), queueIndices.presentQueueIndex, m_surface, &supportsPresentation);
		LP_CORE_ASSERT(supportsPresentation == VK_TRUE, "No queue with presentation support found!");

		// TODO: Query capabilities
		auto device = GraphicsContext::GetDevice();
		m_vulkanDevice = device->GetHandle();
		m_vulkanInstance = GraphicsContext::Get().GetInstance();

		Invalidate(m_width, m_height, true);
	}

	Swapchain::~Swapchain()
	{
		Release();

		m_vulkanInstance = nullptr;
		m_vulkanDevice = nullptr;
	}

	void Swapchain::Invalidate(uint32_t width, uint32_t height, bool useVSync)
	{
		m_width = width;
		m_height = height;

		CreateSwapchain(width, height, useVSync);
		CreateImageViews();
		CreateRenderPass();
		CreateFramebuffers();
		CreateSyncObjects();
		CreateCommandPools();
		CreateCommandBuffers();
	}

	void Swapchain::Release()
	{
		if (!m_swapchain)
		{
			return;
		}

		for (size_t i = 0; i < m_commandPools.size(); i++)
		{
			vkDestroyCommandPool(m_vulkanDevice, m_commandPools[i], nullptr);
		}

		for (uint32_t i = 0; i < m_presentSemaphores.size(); i++)
		{
			vkDestroySemaphore(m_vulkanDevice, m_presentSemaphores[i], nullptr);
			vkDestroySemaphore(m_vulkanDevice, m_renderSemaphores[i], nullptr);
			vkDestroyFence(m_vulkanDevice, m_renderFences[i], nullptr);
		}

		vkDestroyRenderPass(m_vulkanDevice, m_renderPass, nullptr);

		for (uint32_t i = 0; i < m_imageViews.size(); i++)
		{
			vkDestroyFramebuffer(m_vulkanDevice, m_framebuffers[i], nullptr);
			vkDestroyImageView(m_vulkanDevice, m_imageViews[i], nullptr);
		}

		vkDestroySwapchainKHR(m_vulkanDevice, m_swapchain, nullptr);
		vkDestroySurfaceKHR(m_vulkanInstance, m_surface, nullptr);
	}

	void Swapchain::BeginFrame()
	{
		auto device = GraphicsContext::GetDevice()->GetHandle();

		LP_VK_CHECK(vkWaitForFences(device, 1, &m_renderFences[m_currentFrame], VK_TRUE, 1000000000));
		LP_VK_CHECK(vkResetFences(device, 1, &m_renderFences[m_currentFrame]));

		LP_VK_CHECK(vkAcquireNextImageKHR(device, m_swapchain, 1000000000, m_presentSemaphores[m_currentFrame], nullptr, &m_currentImage));
		LP_VK_CHECK(vkResetCommandPool(device, m_commandPools[m_currentFrame], 0));
	}

	void Swapchain::Present()
	{
		// Queue Submit
		{
			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];

			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &m_renderSemaphores[m_currentFrame];

			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &m_presentSemaphores[m_currentFrame];

			const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			submitInfo.pWaitDstStageMask = &waitStage;

			LP_VK_CHECK(vkQueueSubmit(GraphicsContext::GetDevice()->GetGraphicsQueue(), 1, &submitInfo, m_renderFences[m_currentFrame]));
		}

		// Present to screen
		{
			VkPresentInfoKHR presentInfo{};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &m_swapchain;

			presentInfo.pWaitSemaphores = &m_renderSemaphores[m_currentFrame];
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pImageIndices = &m_currentImage;

			LP_VK_CHECK(vkQueuePresentKHR(GraphicsContext::GetDevice()->GetGraphicsQueue(), &presentInfo));
		}

		m_currentFrame = (m_currentFrame + 1) % m_framesInFlight;
	}

	void Swapchain::Resize(uint32_t width, uint32_t height, bool useVSync)
	{
		if (!m_swapchain)
		{
			return;
		}

		m_width = width;
		m_height = height;
		
		CreateSwapchain(width, height, useVSync);
		CreateImageViews();
		CreateFramebuffers();
	}

	Ref<Swapchain> Swapchain::Create(GLFWwindow* window)
	{
		return CreateRef<Swapchain>(window);
	}
	
	void Swapchain::CreateSwapchain(uint32_t width, uint32_t height, bool useVSync)
	{
		const VkSurfaceFormatKHR surfaceFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		
		VkPresentModeKHR presentMode;
		if (useVSync)
		{
			presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		}
		else
		{
			presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		}

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(GraphicsContext::GetPhysicalDevice()->GetHandle(), m_surface, &surfaceCapabilities);

		m_imageCount = surfaceCapabilities.minImageCount + 1;
		if (surfaceCapabilities.maxImageCount > 0 && m_imageCount > surfaceCapabilities.maxImageCount)
		{
			m_imageCount = surfaceCapabilities.maxImageCount;
		}

		VkSwapchainKHR oldSwapchain = m_swapchain;

		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.surface = m_surface;
		swapchainCreateInfo.minImageCount = m_imageCount;
		swapchainCreateInfo.imageFormat = surfaceFormat.format;
		swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent.width = width;
		swapchainCreateInfo.imageExtent.height = height;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		swapchainCreateInfo.oldSwapchain = oldSwapchain;

		// Set sharing mode based on queues
		{
			const auto queueIndices = GraphicsContext::GetPhysicalDevice()->GetQueueIndices();

			if (queueIndices.graphicsQueueIndex != queueIndices.presentQueueIndex)
			{
				swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			}
			else
			{
				swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			}
		}

		auto device = GraphicsContext::GetDevice();
		LP_VK_CHECK(vkCreateSwapchainKHR(device->GetHandle(), &swapchainCreateInfo, nullptr, &m_swapchain));

		if (oldSwapchain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(m_vulkanDevice, oldSwapchain, nullptr);
		
			for (auto& view : m_imageViews)
			{
				vkDestroyImageView(m_vulkanDevice, view, nullptr);
			}

			for (auto& framebuffer : m_framebuffers)
			{
				vkDestroyFramebuffer(m_vulkanDevice, framebuffer, nullptr);
			}

			m_imageViews.clear();
			m_framebuffers.clear();
		}
		
		LP_VK_CHECK(vkGetSwapchainImagesKHR(device->GetHandle(), m_swapchain, &m_imageCount, nullptr));
		m_images.resize(m_imageCount);
		LP_VK_CHECK(vkGetSwapchainImagesKHR(device->GetHandle(), m_swapchain, &m_imageCount, m_images.data()));
		
		m_format = surfaceFormat.format;
	}
	
	void Swapchain::CreateImageViews()
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.layerCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.levelCount = 1;

		m_imageViews.resize(m_images.size());
		for (size_t i = 0; i < m_imageViews.size(); i++)
		{
			createInfo.image = m_images[i];
			LP_VK_CHECK(vkCreateImageView(GraphicsContext::GetDevice()->GetHandle(), &createInfo, nullptr, &m_imageViews[i]));
		}
	}
	
	void Swapchain::CreateRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference attachmentRef{};
		attachmentRef.attachment = 0;
		attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDesc{};
		subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDesc.colorAttachmentCount = 1;
		subpassDesc.pColorAttachments = &attachmentRef;
		subpassDesc.pDepthStencilAttachment = nullptr;

		VkSubpassDependency subpassDepend{};
		subpassDepend.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDepend.dstSubpass = 0;
		subpassDepend.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpassDepend.srcAccessMask = 0;
		subpassDepend.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpassDepend.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &colorAttachment;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpassDesc;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &subpassDepend;

		LP_VK_CHECK(vkCreateRenderPass(GraphicsContext::GetDevice()->GetHandle(), &createInfo, nullptr, &m_renderPass));
	}
	
	void Swapchain::CreateFramebuffers()
	{
		VkFramebufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = m_renderPass;
		createInfo.attachmentCount = 1;
		createInfo.width = m_width;
		createInfo.height = m_height;
		createInfo.layers = 1;

		m_framebuffers.resize(m_images.size());
		for (size_t i = 0; i < m_framebuffers.size(); i++)
		{
			createInfo.pAttachments = &m_imageViews[i];
			LP_VK_CHECK(vkCreateFramebuffer(GraphicsContext::GetDevice()->GetHandle(), &createInfo, nullptr, &m_framebuffers[i]));
		}
	}

	void Swapchain::CreateSyncObjects()
	{
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		auto device = GraphicsContext::GetDevice()->GetHandle();

		m_renderFences.resize(m_framesInFlight);
		for (size_t i = 0; i < m_renderFences.size(); i++)
		{
			LP_VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &m_renderFences[i]));
		}

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		m_presentSemaphores.resize(m_framesInFlight);
		m_renderSemaphores.resize(m_framesInFlight);

		for (size_t i = 0; i < m_presentSemaphores.size(); i++)
		{
			LP_VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_presentSemaphores[i]));
			LP_VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_renderSemaphores[i]));
		}
	}

	void Swapchain::CreateCommandPools()
	{
		const auto queueIndices = GraphicsContext::GetPhysicalDevice()->GetQueueIndices();

		VkCommandPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.queueFamilyIndex = queueIndices.graphicsQueueIndex;
		createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

		m_commandPools.resize(m_framesInFlight);

		for (size_t i = 0; i < m_commandPools.size(); i++)
		{
			LP_VK_CHECK(vkCreateCommandPool(GraphicsContext::GetDevice()->GetHandle(), &createInfo, nullptr, &m_commandPools[i]));
		}
	}

	void Swapchain::CreateCommandBuffers()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandBufferCount = 1;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		m_commandBuffers.resize(m_framesInFlight);
		for (size_t i = 0; i < m_commandBuffers.size(); i++)
		{
			allocInfo.commandPool = m_commandPools[i];
			LP_VK_CHECK(vkAllocateCommandBuffers(GraphicsContext::GetDevice()->GetHandle(), &allocInfo, &m_commandBuffers[i]));
		}
	}
}