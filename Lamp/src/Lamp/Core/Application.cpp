#include "lppch.h"
#include "Application.h"

#include "Lamp/Log/Log.h"
#include "Lamp/Core/Base.h"
#include "Lamp/Core/Window.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"
#include "Lamp/Core/Graphics/Swapchain.h"

#include "Lamp/Asset/AssetManager.h"
#include "Lamp/Asset/Mesh/Mesh.h"
#include "Lamp/Asset/Mesh/Material.h"

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

#include <vulkan/vulkan.h>
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

		m_renderPass = AssetManager::GetAsset<RenderPass>("Engine/RenderPasses/forward.lprp");
	}

	Application::~Application()
	{
		vkDeviceWaitIdle(GraphicsContext::GetDevice()->GetHandle());

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

			// Swapchain
			{
				VkCommandBuffer cmdBuffer = m_window->GetSwapchain().GetCurrentCommandBuffer();
				uint32_t currentFrame = m_window->GetSwapchain().GetCurrentFrame();

				VkCommandBufferBeginInfo cmdBufferBegin{};
				cmdBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				cmdBufferBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				LP_VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBegin));

				VkClearValue clearValue{};
				clearValue.color = { { 1.f, 0.f, 1.f, 1.f } };
				VkRenderPassBeginInfo renderPassBegin{};
				renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassBegin.clearValueCount = 1;
				renderPassBegin.pClearValues = &clearValue;
				renderPassBegin.framebuffer = m_window->GetSwapchain().GetCurrentFramebuffer();
				renderPassBegin.renderPass = m_window->GetSwapchain().GetRenderPass();
				renderPassBegin.renderArea.extent = { m_window->GetWidth(), m_window->GetHeight() };
				renderPassBegin.renderArea.offset = { 0, 0 };
				vkCmdBeginRenderPass(cmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdEndRenderPass(cmdBuffer);
				LP_VK_CHECK(vkEndCommandBuffer(cmdBuffer));
			}

			m_window->Present();
		}
	}

	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowCloseEvent>(LP_BIND_EVENT_FN(Application::OnWindowCloseEvent));
		dispatcher.Dispatch<WindowResizeEvent>(LP_BIND_EVENT_FN(Application::OnWindowResizeEvent));
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
		return false;
	}
}
