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

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>

namespace Lamp
{
	static bool LoadShaderModule(const std::filesystem::path& path, VkDevice device, VkShaderModule& outShaderModule)
	{
		std::ifstream file(path, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			return false;
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

		file.seekg(0);
		file.read((char*)buffer.data(), fileSize);
		file.close();

		VkShaderModuleCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.codeSize = fileSize;
		info.pCode = buffer.data();

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &info, nullptr, &shaderModule) != VK_SUCCESS) [[unlikely]]
		{
			return false;
		}

		outShaderModule = shaderModule;
		return true;
	}

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

		auto mesh = AssetManager::GetAsset<Mesh>("Assets/SM_Particle_Chest.fbx");

		CreatePipeline();

		
	}

	Application::~Application()
	{
		VulkanDeletionQueue::Flush();

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

			VkCommandBufferBeginInfo cmdBufferBegin{};
			cmdBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufferBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			LP_VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBegin));

			// Begin RenderPass
			{
				VkClearValue clearValue;
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
			}

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

			const glm::vec3 camPos = { 0.f, 0.f, -2.f };
			const glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
			glm::mat4 projection = glm::perspective(glm::radians(70.f), 1280.f / 720.f, 0.1f, 1000.f);
			projection[1][1] *= -1.f;
			
			const glm::mat4 model = glm::rotate(glm::mat4{ 1.f }, glm::radians(m_frameNumber * 0.4f), glm::vec3{ 0.f, 1.f, 0.f });
			const glm::mat4 transform = projection * view * model;

			MeshPushConstants constants;
			constants.transform = transform;

			vkCmdPushConstants(cmdBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_vertexBuffer->)
			vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

			vkCmdEndRenderPass(cmdBuffer);
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

		VkShaderModule fragShader;
		VkShaderModule vertShader;

		if (!LoadShaderModule("Assets/Shaders/triangle.frag.spv", device, fragShader)) [[unlikely]]
		{
			std::cout << "Error when building the triangle fragment shader module" << std::endl;
		}
		else [[likely]]
		{
			std::cout << "Triangle fragment shader successfully loaded" << std::endl;
		}

		if (!LoadShaderModule("Assets/Shaders/tri_mesh.vert.spv", device, vertShader)) [[unlikely]]
		{
			std::cout << "Error when building the triangle vertex shader module" << std::endl;
		}
		else [[likely]]
		{
			std::cout << "Triangle vertex shader successfully loaded" << std::endl;
		}

		// Pipeline layout
		{
			VkPushConstantRange pushConstantRange{};
			pushConstantRange.offset = 0;
			pushConstantRange.size = sizeof(MeshPushConstants);
			pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 0;
			pipelineLayoutInfo.pSetLayouts = nullptr;
			pipelineLayoutInfo.pushConstantRangeCount = 1;
			pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

			LP_VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));
		}

		std::vector<VkVertexInputBindingDescription> vertexBindings;
		std::vector<VkVertexInputAttributeDescription> vertexAttributes;
		
		VkPipelineVertexInputStateCreateFlags vertexInputStateFlags = 0;
		
		// Vertex input desc
		{
			{
				auto& vertInput = vertexBindings.emplace_back();
				vertInput.binding = 0;
				vertInput.stride = sizeof(Vertex);
				vertInput.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			}

			// Position
			{
				auto& vertAttr = vertexAttributes.emplace_back();
				vertAttr.binding = 0;
				vertAttr.location = 0;
				vertAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
				vertAttr.offset = offsetof(Vertex, Vertex::position);
			}

			// Normal
			{
				auto& vertAttr = vertexAttributes.emplace_back();
				vertAttr.binding = 0;
				vertAttr.location = 1;
				vertAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
				vertAttr.offset = offsetof(Vertex, Vertex::normal);
			}

			// Tangent
			{
				auto& vertAttr = vertexAttributes.emplace_back();
				vertAttr.binding = 0;
				vertAttr.location = 2;
				vertAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
				vertAttr.offset = offsetof(Vertex, Vertex::tangent);
			}

			// Bitangent
			{
				auto& vertAttr = vertexAttributes.emplace_back();
				vertAttr.binding = 0;
				vertAttr.location = 3;
				vertAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
				vertAttr.offset = offsetof(Vertex, Vertex::bitangent);
			}

			// Texture coords
			{
				auto& vertAttr = vertexAttributes.emplace_back();
				vertAttr.binding = 0;
				vertAttr.location = 4;
				vertAttr.format = VK_FORMAT_R32G32_SFLOAT;
				vertAttr.offset = offsetof(Vertex, Vertex::textureCoords);
			}
		}

		// Pipeline
		{
			VkPipelineVertexInputStateCreateInfo vertInputState{};
			vertInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertInputState.vertexBindingDescriptionCount = vertexBindings.size();
			vertInputState.pVertexBindingDescriptions = vertexBindings.data();

			vertInputState.vertexAttributeDescriptionCount = vertexAttributes.size();
			vertInputState.pVertexAttributeDescriptions = vertexAttributes.data();

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
			inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyState.primitiveRestartEnable = VK_FALSE;
			inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

			VkViewport viewport{};
			viewport.width = (float)m_applicationInfo.width;
			viewport.height = (float)m_applicationInfo.height;
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;
			viewport.x = 0;
			viewport.y = 0;

			VkRect2D scissor{};
			scissor.extent = { m_applicationInfo.width, m_applicationInfo.height };
			scissor.offset = { 0, 0 };

			VkPipelineRasterizationStateCreateInfo rasterizationState{};
			rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationState.cullMode = VK_CULL_MODE_NONE;
			rasterizationState.depthBiasClamp = VK_FALSE;
			rasterizationState.rasterizerDiscardEnable = VK_FALSE;
			rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
			rasterizationState.depthBiasEnable = VK_FALSE;
			rasterizationState.depthBiasConstantFactor = 0.f;
			rasterizationState.depthBiasClamp = 0.f;
			rasterizationState.depthBiasSlopeFactor = 0.f;
			rasterizationState.lineWidth = 1.f;

			VkPipelineMultisampleStateCreateInfo multisampleState{};
			multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleState.sampleShadingEnable = VK_FALSE;
			multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisampleState.minSampleShading = 1.f;
			multisampleState.pSampleMask = nullptr;
			multisampleState.alphaToCoverageEnable = VK_FALSE;
			multisampleState.alphaToOneEnable = VK_FALSE;

			VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
			colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachmentState.blendEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportState{};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.pNext = nullptr;
			viewportState.viewportCount = 1;
			viewportState.pViewports = &viewport;
			viewportState.scissorCount = 1;
			viewportState.pScissors = &scissor;

			VkPipelineColorBlendStateCreateInfo blendState{};
			blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			blendState.logicOpEnable = VK_FALSE;
			blendState.attachmentCount = 1;
			blendState.pAttachments = &colorBlendAttachmentState;
			blendState.logicOp = VK_LOGIC_OP_COPY;

			VkPipelineShaderStageCreateInfo shaderStages[2]{};
			{
				shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
				shaderStages[0].module = vertShader;
				shaderStages[0].pName = "main";
				shaderStages[0].pSpecializationInfo = nullptr;
				shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				shaderStages[1].module = fragShader;
				shaderStages[1].pName = "main";
				shaderStages[1].pSpecializationInfo = nullptr;
			}

			VkGraphicsPipelineCreateInfo pipelineInfo{};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.stageCount = 2;
			pipelineInfo.pStages = &shaderStages[0];
			pipelineInfo.pVertexInputState = &vertInputState;
			pipelineInfo.pInputAssemblyState = &inputAssemblyState;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizationState;
			pipelineInfo.pMultisampleState = &multisampleState;
			pipelineInfo.pColorBlendState = &blendState;
			pipelineInfo.layout = m_pipelineLayout;
			pipelineInfo.renderPass = m_window->GetSwapchain().GetRenderPass();
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

			LP_VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline));
		}

		vkDestroyShaderModule(device, vertShader, nullptr);
		vkDestroyShaderModule(device, fragShader, nullptr);
		
		VulkanDeletionQueue::Push([this]()
			{
				auto device = GraphicsContext::GetDevice()->GetHandle();
				vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
				vkDestroyPipeline(device, m_pipeline, nullptr);
			});
	}
	
	void Application::CreateTriangle()
	{
		std::vector<Vertex> vertices = 
		{ 
			{{ 1.f, 1.f, 0.f }},
			{{ -1.f, 1.f, 0.f }},
			{{ 0.f, -1.f, 0.f }}
		};
	}
}