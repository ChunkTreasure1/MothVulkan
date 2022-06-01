#include "lppch.h"
#include "Application.h"

#include "Lamp/Log/Log.h"
#include "Lamp/Core/Base.h"
#include "Lamp/Core/Window.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"
#include "Lamp/Core/Graphics/Swapchain.h"
#include "Lamp/Core/Graphics/VulkanDeletionQueue.h"

#include "Lamp/Asset/AssetManager.h"
#include "Lamp/Asset/Mesh/Mesh.h"

#include "Lamp/Rendering/Buffer/VertexBuffer.h"
#include "Lamp/Rendering/Buffer/IndexBuffer.h"
#include "Lamp/Rendering/Buffer/UniformBufferSet.h"
#include "Lamp/Rendering/Buffer/UniformBuffer.h"
#include "Lamp/Rendering/Buffer/ShaderStorageBufferSet.h"
#include "Lamp/Rendering/Buffer/ShaderStorageBuffer.h"

#include "Lamp/Rendering/Texture/Texture2D.h"
#include "Lamp/Rendering/RenderPipeline.h"
#include "Lamp/Rendering/Framebuffer.h"

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

		m_mesh = AssetManager::GetAsset<Mesh>("Assets/SM_Particle_Chest.fbx");
		m_texture = AssetManager::GetAsset<Texture2D>("Assets/Textures/peter_lambert.png");
		m_shader = AssetManager::GetAsset<Shader>("Engine/Shaders/Definitions/trimesh.lpsdef");

		CreateDescriptors();
		CreatePipeline();
	}

	Application::~Application()
	{
		vkDeviceWaitIdle(GraphicsContext::GetDevice()->GetHandle());

		VulkanDeletionQueue::Flush();

		m_assetManager = nullptr;
		m_mesh = nullptr;
		m_texture = nullptr;
		
		m_renderPipeline = nullptr;
		m_shader = nullptr;
		m_framebuffer = nullptr;
		
		m_uniformBufferSet = nullptr;
		m_objectBufferSet = nullptr;

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
				auto currentCameraBuffer = m_uniformBufferSet->Get(currentFrame);
				CameraData* cameraData = currentCameraBuffer->Map<CameraData>();

				cameraData->proj = projection;
				cameraData->view = view;
				cameraData->viewProj = projection * view;

				currentCameraBuffer->Unmap();
			}

			// Update object data
			{
				auto currentObjectBuffer = m_objectBufferSet->Get(currentFrame);
				ObjectData* objectData = currentObjectBuffer->Map<ObjectData>();

				objectData[0].transform = transform;

				currentObjectBuffer->Unmap();
			}

			m_renderPipeline->Bind(cmdBuffer);

			for (size_t i = 0; i < m_frameDescriptorSets[currentFrame].size(); i++)
			{
				m_renderPipeline->BindDescriptorSet(cmdBuffer, m_frameDescriptorSets[currentFrame][i], (uint32_t)i);
			}

			// Draw
			{
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

		FramebufferSpecification framebufferSpec{};
		framebufferSpec.width = 1280;
		framebufferSpec.height = 720;
		framebufferSpec.attachments =
		{
			ImageFormat::RGBA,
			ImageFormat::DEPTH32F
		};

		m_framebuffer = Framebuffer::Create(framebufferSpec);

		RenderPipelineSpecification renderSpec{};
		renderSpec.shader = m_shader;
		renderSpec.framebuffer = m_framebuffer;
		renderSpec.vertexLayout =
		{
			{ ElementType::Float3, "a_position" },
			{ ElementType::Float3, "a_normal" },
			{ ElementType::Float3, "a_tangent" },
			{ ElementType::Float3, "a_bitangent" },
			{ ElementType::Float2, "a_texCoords" }
		};

		m_renderPipeline = RenderPipeline::Create(renderSpec);
	}

	void Application::CreateDescriptors()
	{
		const uint32_t framesInFlight = m_window->GetSwapchain().GetFramesInFlight();
		m_uniformBufferSet = UniformBufferSet::Create(sizeof(CameraData), framesInFlight);

		constexpr uint32_t MAX_OBJECT_COUNT = 10000;
		m_objectBufferSet = ShaderStorageBufferSet::Create(sizeof(ObjectData) * MAX_OBJECT_COUNT, framesInFlight);

		// Create descriptor pool
		{
			std::vector<VkDescriptorPoolSize> sizes =
			{
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 }
			};

			VkDescriptorPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.flags = 0;
			poolInfo.maxSets = 10;
			poolInfo.poolSizeCount = (uint32_t)sizes.size();
			poolInfo.pPoolSizes = sizes.data();

			vkCreateDescriptorPool(GraphicsContext::GetDevice()->GetHandle(), &poolInfo, nullptr, &m_descriptorPool);

			VulkanDeletionQueue::Push([&]()
				{
					vkDestroyDescriptorPool(GraphicsContext::GetDevice()->GetHandle(), m_descriptorPool, nullptr);
				});
		}

		m_shaderResources = m_shader->GetResources();

		// Allocate descriptor sets
		{
			m_frameDescriptorSets.resize(framesInFlight);
			for (uint32_t i = 0; i < m_frameDescriptorSets.size(); i++)
			{
				auto& sets = m_frameDescriptorSets[i];

				VkDescriptorSetAllocateInfo allocInfo = m_shader->GetResources().setAllocInfo;
				allocInfo.descriptorPool = m_descriptorPool;
				sets.resize(allocInfo.descriptorSetCount);

				vkAllocateDescriptorSets(GraphicsContext::GetDevice()->GetHandle(), &allocInfo, sets.data());

				std::vector<VkWriteDescriptorSet> writeDescriptors;

				for (auto& [set, bindings] : m_shaderResources.writeDescriptors)
				{
					for (auto& [binding, write] : bindings)
					{
						if (m_shaderResources.uniformBuffersInfos[set].find(binding) != m_shaderResources.uniformBuffersInfos[set].end())
						{
							write.pBufferInfo = &m_shaderResources.uniformBuffersInfos[set][binding];
						}
						else if (m_shaderResources.storageBuffersInfos[set].find(binding) != m_shaderResources.storageBuffersInfos[set].end())
						{
							write.pBufferInfo = &m_shaderResources.storageBuffersInfos[set][binding];
						}
						else if (m_shaderResources.imageInfos[set].find(binding) != m_shaderResources.imageInfos[set].end())
						{
							write.pImageInfo = &m_shaderResources.imageInfos[set][binding];
						}
					}
				}
				m_shaderResources.uniformBuffersInfos[0][0].buffer = m_uniformBufferSet->Get(i)->GetHandle();
				m_shaderResources.writeDescriptors[0][0].dstSet = sets[0];

				m_shaderResources.storageBuffersInfos[1][0].buffer = m_objectBufferSet->Get(i)->GetHandle();
				m_shaderResources.storageBuffersInfos[1][0].range = sizeof(ObjectData) * MAX_OBJECT_COUNT;
				m_shaderResources.writeDescriptors[1][0].dstSet = sets[1];

				m_shaderResources.imageInfos[2][0].imageView = m_texture->GetImage()->GetView();
				m_shaderResources.imageInfos[2][0].sampler = m_texture->GetImage()->GetSampler();
				m_shaderResources.writeDescriptors[2][0].dstSet = sets[2];

				writeDescriptors.emplace_back(m_shaderResources.writeDescriptors[0][0]);
				writeDescriptors.emplace_back(m_shaderResources.writeDescriptors[1][0]);
				writeDescriptors.emplace_back(m_shaderResources.writeDescriptors[2][0]);

				vkUpdateDescriptorSets(GraphicsContext::GetDevice()->GetHandle(), (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
			}
		}
	}
}
