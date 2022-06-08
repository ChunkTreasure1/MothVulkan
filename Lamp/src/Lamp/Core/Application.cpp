#include "lppch.h"
#include "Application.h"

#include "Lamp/Log/Log.h"
#include "Lamp/Core/Base.h"
#include "Lamp/Core/Window.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"
#include "Lamp/Core/Graphics/Swapchain.h"
#include "Lamp/Core/Layer/Layer.h"

#include "Lamp/Asset/AssetManager.h"
#include "Lamp/Asset/Mesh/Mesh.h"
#include "Lamp/Asset/Mesh/Material.h"

#include "Lamp/ImGui/ImGuiImplementation.h"

#include "Lamp/Rendering/Buffer/VertexBuffer.h"
#include "Lamp/Rendering/Buffer/IndexBuffer.h"

#include "Lamp/Rendering/Buffer/UniformBuffer/UniformBufferSet.h"
#include "Lamp/Rendering/Buffer/UniformBuffer/UniformBufferRegistry.h"

#include "Lamp/Rendering/Buffer/ShaderStorageBuffer/ShaderStorageBufferSet.h"
#include "Lamp/Rendering/Buffer/ShaderStorageBuffer/ShaderStorageBufferRegistry.h"

#include "Lamp/Rendering/Texture/Texture2D.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipeline.h"
#include "Lamp/Rendering/Framebuffer.h"

#include "Lamp/Rendering/Shader/ShaderRegistry.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipelineRegistry.h"
#include "Lamp/Rendering/RenderPass/RenderPassRegistry.h"
#include "Lamp/Rendering/Renderer.h"
#include "Lamp/Rendering/RenderPass/RenderPass.h"

#include "Lamp/Utility/ImageUtility.h"

#include <vulkan/vulkan.h>
#include <imgui.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>

namespace Lamp
{
	Application::Application(const ApplicationInfo& info)
	{
		LP_CORE_ASSERT(!s_instance, "Application already exists!");
		s_instance = this;

		m_applicationInfo = info;

		WindowProperties windowProperties{};
		windowProperties.width = info.width;
		windowProperties.height = info.height;
		windowProperties.vsync = info.useVSync;
		windowProperties.title = info.title;

		m_window = Window::Create(windowProperties);
		m_window->SetEventCallback(LP_BIND_EVENT_FN(Application::OnEvent));

		m_assetManager = CreateRef<AssetManager>();

		UniformBufferRegistry::Initialize();
		ShaderStorageBufferRegistry::Initialize();

		const uint32_t framesInFlight = m_window->GetSwapchain().GetFramesInFlight();
		constexpr uint32_t MAX_OBJECT_COUNT = 10000;

		UniformBufferRegistry::Register(0, 0, UniformBufferSet::Create(sizeof(CameraData), framesInFlight));
		ShaderStorageBufferRegistry::Register(1, 0, ShaderStorageBufferSet::Create(sizeof(ObjectData) * MAX_OBJECT_COUNT, framesInFlight));

		ShaderRegistry::Initialize();
		RenderPassRegistry::Initialize();
		RenderPipelineRegistry::Initialize();
		RenderPassRegistry::SetupOverrides();

		Renderer::Initialize();

		m_imguiImplementation = ImGuiImplementation::Create();

		m_renderPass = AssetManager::GetAsset<RenderPass>("Engine/RenderPasses/forward.lprp");
	}

	Application::~Application()
	{
		vkDeviceWaitIdle(GraphicsContext::GetDevice()->GetHandle());
		
		m_layerStack.Clear();
		m_imguiImplementation = nullptr;

		Renderer::Shutdowm();

		ShaderStorageBufferRegistry::Shutdowm();
		UniformBufferRegistry::Shutdowm();
		RenderPipelineRegistry::Shutdown();
		RenderPassRegistry::Shutdown();
		ShaderRegistry::Shutdown();

		m_renderPass = nullptr;
		m_assetManager = nullptr;
		m_window = nullptr;

		s_instance = nullptr;
	}

	void Application::Run()
	{
		auto device = GraphicsContext::GetDevice()->GetHandle();

		while (m_isRunning)
		{
			m_window->BeginFrame();

			Renderer::Begin();
			Renderer::BeginPass(m_renderPass);

			Renderer::Draw();

			Renderer::EndPass();

			if (glfwGetKey(m_window->GetNativeWindow(), GLFW_KEY_R) == GLFW_PRESS)
			{
				Renderer::TEST_RecompileShader();
			}

			Renderer::End();

			//// Swapchain
			//{
			//	VkCommandBuffer cmdBuffer = m_window->GetSwapchain().GetCurrentCommandBuffer();
			//	uint32_t currentFrame = m_window->GetSwapchain().GetCurrentFrame();

			//	VkCommandBufferBeginInfo cmdBufferBegin{};
			//	cmdBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			//	cmdBufferBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			//	LP_VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBegin));

			//	{
			//		VkImageCopy imageCopy{};
			//		imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			//		imageCopy.srcSubresource.baseArrayLayer = 0;
			//		imageCopy.srcSubresource.layerCount = 1;
			//		imageCopy.srcSubresource.mipLevel = 0;

			//		imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			//		imageCopy.dstSubresource.baseArrayLayer = 0;
			//		imageCopy.dstSubresource.layerCount = 1;
			//		imageCopy.dstSubresource.mipLevel = 0;

			//		imageCopy.srcOffset.x = 0;
			//		imageCopy.srcOffset.y = 0;
			//		imageCopy.srcOffset.z = 0;

			//		imageCopy.dstOffset.x = 0;
			//		imageCopy.dstOffset.y = 0;
			//		imageCopy.dstOffset.z = 0;

			//		imageCopy.extent.width = m_window->GetWidth();
			//		imageCopy.extent.height = m_window->GetHeight();
			//		imageCopy.extent.depth = 1;

			//		Ref<Image2D> attachmentImage = m_renderPass->framebuffer->GetColorAttachment(0);
			//		VkImage swapchainImage = m_window->GetSwapchain().GetCurrentImage();

			//		attachmentImage->TransitionToLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			//		const VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			//		Utility::TransitionImageLayout(cmdBuffer, swapchainImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);

			//		vkCmdCopyImage(cmdBuffer, m_renderPass->framebuffer->GetColorAttachment(0)->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			//			m_window->GetSwapchain().GetCurrentImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

			//		Utility::TransitionImageLayout(cmdBuffer, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, range);
			//		attachmentImage->TransitionToLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			//	}

			//	VkClearValue clearValue{};
			//	clearValue.color = { { 1.f, 0.f, 1.f, 1.f } };
			//	VkRenderPassBeginInfo renderPassBegin{};
			//	renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			//	renderPassBegin.clearValueCount = 1;
			//	renderPassBegin.pClearValues = &clearValue;
			//	renderPassBegin.framebuffer = m_window->GetSwapchain().GetCurrentFramebuffer();
			//	renderPassBegin.renderPass = m_window->GetSwapchain().GetRenderPass();
			//	renderPassBegin.renderArea.extent = { m_window->GetWidth(), m_window->GetHeight() };
			//	renderPassBegin.renderArea.offset = { 0, 0 };
			//	vkCmdBeginRenderPass(cmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
			//	vkCmdEndRenderPass(cmdBuffer);

			//	LP_VK_CHECK(vkEndCommandBuffer(cmdBuffer));

			//}
			
			m_imguiImplementation->Begin();
			
			AppImGuiUpdateEvent imguiEvent{};
			OnEvent(imguiEvent);
			
			m_imguiImplementation->End();

			m_window->Present();
		}
	}

	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowCloseEvent>(LP_BIND_EVENT_FN(Application::OnWindowCloseEvent));
		dispatcher.Dispatch<WindowResizeEvent>(LP_BIND_EVENT_FN(Application::OnWindowResizeEvent));

		//Handle rest of events
		for (auto it = m_layerStack.end(); it != m_layerStack.begin(); )
		{
			(*--it)->OnEvent(event);
			if (event.handled)
			{
				break;
			}
		}
	}

	void Application::PushLayer(Layer* layer)
	{
		m_layerStack.PushLayer(layer);
	}

	bool Application::OnWindowCloseEvent(WindowCloseEvent& e)
	{
		m_isRunning = false;
		return true;
	}

	bool Application::OnWindowResizeEvent(WindowResizeEvent& e)
	{
		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			return false;
		}

		m_window->Resize(e.GetWidth(), e.GetHeight());
		m_renderPass->framebuffer->Resize(e.GetWidth(), e.GetHeight());
		return false;
	}
}
