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
#include "Lamp/Asset/Mesh/MaterialInstance.h"

#include "Lamp/Rendering/Buffer/VertexBuffer.h"
#include "Lamp/Rendering/Buffer/IndexBuffer.h"

#include "Lamp/Rendering/Buffer/UniformBufferSet.h"
#include "Lamp/Rendering/Buffer/UniformBuffer.h"
#include "Lamp/Rendering/Buffer/UniformBufferRegistry.h"

#include "Lamp/Rendering/Buffer/ShaderStorageBufferSet.h"
#include "Lamp/Rendering/Buffer/ShaderStorageBuffer.h"
#include "Lamp/Rendering/Buffer/ShaderStorageBufferRegistry.h"

#include "Lamp/Rendering/Texture/Texture2D.h"
#include "Lamp/Rendering/RenderPipeline.h"
#include "Lamp/Rendering/Framebuffer.h"

#include "Lamp/Rendering/Shader/ShaderRegistry.h"
#include "Lamp/Rendering/RenderPipelineRegistry.h"

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
		m_assetManager = CreateRef<AssetManager>();

		// Framebuffer
		{
			FramebufferSpecification framebufferSpec{};
			framebufferSpec.width = 1280;
			framebufferSpec.height = 720;
			framebufferSpec.attachments =
			{
				ImageFormat::RGBA,
				ImageFormat::DEPTH32F
			};

			framebufferSpec.attachments[0].clearColor = { 0.f, 0.f, 0.f, 1.f };

			s_framebuffer = Framebuffer::Create(framebufferSpec);
		}

		UniformBufferRegistry::Initialize();
		ShaderStorageBufferRegistry::Initialize();

		const uint32_t framesInFlight = m_window->GetSwapchain().GetFramesInFlight();
		constexpr uint32_t MAX_OBJECT_COUNT = 10000;

		UniformBufferRegistry::Register(0, 0, UniformBufferSet::Create(sizeof(CameraData), framesInFlight));
		ShaderStorageBufferRegistry::Register(1, 0, ShaderStorageBufferSet::Create(sizeof(ObjectData) * MAX_OBJECT_COUNT, framesInFlight));

		ShaderRegistry::Initialize();
		RenderPipelineRegistry::Initialize();

		m_mesh = AssetManager::GetAsset<Mesh>("Assets/SM_Particle_Chest.fbx");
		m_texture = AssetManager::GetAsset<Texture2D>("Assets/Textures/peter_lambert.png");

		CreatePipeline();
	}

	Application::~Application()
	{
		vkDeviceWaitIdle(GraphicsContext::GetDevice()->GetHandle());

		m_materialInstance = nullptr;
		m_material = nullptr;
		m_mesh = nullptr;
		m_texture = nullptr;

		m_renderPipeline = nullptr;
		s_framebuffer = nullptr;

		ShaderStorageBufferRegistry::Shutdowm();
		UniformBufferRegistry::Shutdowm();
		RenderPipelineRegistry::Shutdown();
		ShaderRegistry::Shutdown();

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

			VkCommandBuffer cmdBuffer = m_window->GetSwapchain().GetCurrentCommandBuffer();
			uint32_t currentFrame = m_window->GetSwapchain().GetCurrentFrame();

			VkCommandBufferBeginInfo cmdBufferBegin{};
			cmdBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufferBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			LP_VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBegin));

			// Begin RenderPass
			{
				auto framebuffer = m_renderPipeline->GetSpecification().framebuffer;

				framebuffer->Bind(cmdBuffer);

				VkRenderingInfo renderingInfo{};
				renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
				renderingInfo.renderArea = { 0, 0, framebuffer->GetSpecification().width, framebuffer->GetSpecification().height };
				renderingInfo.layerCount = 1;
				renderingInfo.colorAttachmentCount = (uint32_t)framebuffer->GetColorAttachmentInfos().size();
				renderingInfo.pColorAttachments = framebuffer->GetColorAttachmentInfos().data();

				if (framebuffer->GetDepthAttachment())
				{
					renderingInfo.pDepthAttachment = &framebuffer->GetDepthAttachmentInfo();
				}
				else
				{
					renderingInfo.pDepthAttachment = nullptr;
				}
				renderingInfo.pStencilAttachment = nullptr;

				vkCmdBeginRendering(cmdBuffer, &renderingInfo);
			}

			const glm::vec3 camPos = { 0.f, 0.f, -2.f };
			const glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
			glm::mat4 projection = glm::perspective(glm::radians(70.f), 1280.f / 720.f, 0.1f, 1000.f);

			const glm::mat4 model = glm::rotate(glm::mat4{ 1.f }, glm::radians(m_frameNumber * 0.4f), glm::vec3{ 0.f, 1.f, 0.f }) * glm::scale(glm::mat4(1.f), glm::vec3(0.01f));
			const glm::mat4 transform = model;

			// Update camera data
			{
				auto currentCameraBuffer = UniformBufferRegistry::Get(0, 0)->Get(currentFrame);
				CameraData* cameraData = currentCameraBuffer->Map<CameraData>();

				cameraData->proj = projection;
				cameraData->view = view;
				cameraData->viewProj = projection * view;

				currentCameraBuffer->Unmap();
			}

			// Update object data
			{
				auto currentObjectBuffer = ShaderStorageBufferRegistry::Get(1, 0)->Get(currentFrame);
				ObjectData* objectData = currentObjectBuffer->Map<ObjectData>();

				objectData[0].transform = transform;

				currentObjectBuffer->Unmap();
			}


			// Draw
			{
				m_materialInstance->Bind(cmdBuffer, currentFrame);
				m_mesh->GetVertexBuffer()->Bind(cmdBuffer);
				m_mesh->GetIndexBuffer()->Bind(cmdBuffer);

				for (auto& submesh : m_mesh->GetSubMeshes())
				{
					vkCmdDrawIndexed(cmdBuffer, submesh.indexCount, 1, submesh.indexStartOffset, submesh.vertexStartOffset, 0);
				}
			}

			vkCmdEndRendering(cmdBuffer);
			m_renderPipeline->GetSpecification().framebuffer->Unbind(cmdBuffer);

			// Swapchain
			{
				VkClearValue clearValue{};
				clearValue.color = { { 1.f, 0.f, 1.f, 1.f } };
				VkRenderPassBeginInfo renderPassBegin{};
				renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassBegin.clearValueCount = 1;
				renderPassBegin.pClearValues = &clearValue;
				renderPassBegin.framebuffer = m_window->GetSwapchain().GetCurrentFramebuffer();
				renderPassBegin.renderPass = m_window->GetSwapchain().GetRenderPass();
				renderPassBegin.renderArea.extent = { m_applicationInfo.width, m_applicationInfo.height };
				renderPassBegin.renderArea.offset = { 0, 0 };
				vkCmdBeginRenderPass(cmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdEndRenderPass(cmdBuffer);
			}

			LP_VK_CHECK(vkEndCommandBuffer(cmdBuffer));

			m_window->Present();
			m_frameNumber++;
		}
	}

	void Application::Shutdown()
	{
		m_isRunning = false;
	}

	void Application::CreatePipeline()
	{
		auto device = GraphicsContext::GetDevice()->GetHandle();

		m_renderPipeline = RenderPipelineRegistry::Get("trimesh");

		m_material = Material::Create("Mat", 0, m_renderPipeline);
		m_material->SetTexture(0, m_texture);

		m_materialInstance = MaterialInstance::Create(m_material);
	}
}
