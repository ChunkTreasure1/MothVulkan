#pragma once

#include "Lamp/Core/Base.h"

#include <vulkan/vulkan.h>

struct GLFWwindow;
namespace Lamp
{
	class Swapchain
	{
	public:
		Swapchain(GLFWwindow* window);
		~Swapchain();

		void Release();

		void BeginFrame();
		void Present();

		void Resize(uint32_t width, uint32_t height, bool useVSync);

		inline const uint32_t GetCurrentFrame() const { return m_currentFrame; }
		inline const uint32_t GetFramesInFlight() const { return m_framesInFlight; }

		inline VkRenderPass GetRenderPass() const { return m_renderPass; }
		inline VkFramebuffer GetCurrentFramebuffer() const { return m_framebuffers[m_currentImage]; }
		inline VkCommandBuffer GetCurrentCommandBuffer() const { return m_commandBuffers[m_currentFrame]; }
		inline VkCommandBuffer GetCommandBuffer(uint32_t index) const { return m_commandBuffers[index]; }
		inline VkCommandPool GetCommandPool(uint32_t index) const { return m_commandPools[index]; }

		static Ref<Swapchain> Create(GLFWwindow* window);

	private:
		void Invalidate(uint32_t width, uint32_t height, bool useVSync);
		
		void CreateSwapchain(uint32_t width, uint32_t height, bool useVSync);
		void CreateImageViews();
		void CreateRenderPass();
		void CreateFramebuffers();
		void CreateSyncObjects();
		void CreateCommandPools();
		void CreateCommandBuffers();

		uint32_t m_imageCount = 0;
		uint32_t m_currentImage = 0;

		uint32_t m_currentFrame = 0;

		const uint32_t m_framesInFlight = 2;

		uint32_t m_width = 1280;
		uint32_t m_height = 720;

		std::vector<VkImage> m_images;
		std::vector<VkImageView> m_imageViews;
		std::vector<VkFramebuffer> m_framebuffers;

		std::vector<VkFence> m_renderFences;
		std::vector<VkSemaphore> m_renderSemaphores;
		std::vector<VkSemaphore> m_presentSemaphores;

		std::vector<VkCommandPool> m_commandPools;
		std::vector<VkCommandBuffer> m_commandBuffers;

		VkFormat m_format = VK_FORMAT_UNDEFINED;
		VkRenderPass m_renderPass = nullptr;
		VkSwapchainKHR m_swapchain = nullptr;
		VkSurfaceKHR m_surface = nullptr;

		VkDevice m_vulkanDevice = nullptr;
		VkInstance m_vulkanInstance = nullptr;
	};
}