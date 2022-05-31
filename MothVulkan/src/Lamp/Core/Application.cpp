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

		m_mesh = AssetManager::GetAsset<Mesh>("Assets/SM_Particle_Chest.fbx");
		m_texture = AssetManager::GetAsset<Texture2D>("Assets/Textures/peter_lambert.png");

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

			const glm::vec3 camPos = { 0.f, 0.f, -2.f };
			const glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
			glm::mat4 projection = glm::perspective(glm::radians(70.f), 1280.f / 720.f, 0.1f, 1000.f);
			projection[1][1] *= -1.f;

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

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_globalDescriptorSets[currentFrame], 0, nullptr);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 1, 1, &m_objectDescriptorSets[currentFrame], 0, nullptr);

			// Update push constants
			{
				MeshPushConstants constants;
				constants.transform = transform;
				vkCmdPushConstants(cmdBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);
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
			pipelineLayoutInfo.setLayoutCount = 2;
			
			VkDescriptorSetLayout layouts[] = { m_globalSetLayout, m_objectSetLayout };

			pipelineLayoutInfo.pSetLayouts = &m_globalSetLayout;

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
				vertInputState.vertexBindingDescriptionCount = (uint32_t)vertexBindings.size();
				vertInputState.pVertexBindingDescriptions = vertexBindings.data();

				vertInputState.vertexAttributeDescriptionCount = (uint32_t)vertexAttributes.size();
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

	void Application::CreateDescriptors()
	{
		const uint32_t framesInFlight = m_window->GetSwapchain().GetFramesInFlight();
		m_uniformBufferSet = UniformBufferSet::Create(sizeof(CameraData), framesInFlight);

		constexpr uint32_t MAX_OBJECT_COUNT = 10000;
		m_objectBufferSet = ShaderStorageBufferSet::Create(sizeof(ObjectData) * MAX_OBJECT_COUNT, framesInFlight);

		// Create camera descriptor set layout
		{
			VkDescriptorSetLayoutBinding uboBinding{};
			uboBinding.binding = 0;
			uboBinding.descriptorCount = 1;
			uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
			layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutCreateInfo.pNext = nullptr;
			layoutCreateInfo.flags = 0;

			layoutCreateInfo.bindingCount = 1;
			layoutCreateInfo.pBindings = &uboBinding;

			vkCreateDescriptorSetLayout(GraphicsContext::GetDevice()->GetHandle(), &layoutCreateInfo, nullptr, &m_globalSetLayout);

			VulkanDeletionQueue::Push([&]()
				{
					vkDestroyDescriptorSetLayout(GraphicsContext::GetDevice()->GetHandle(), m_globalSetLayout, nullptr);
				});
		}

		// Create object descriptor set layout
		{
			VkDescriptorSetLayoutBinding objectBinding{};
			objectBinding.binding = 0;
			objectBinding.descriptorCount = 1;
			objectBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			objectBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
			layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutCreateInfo.pNext = nullptr;
			layoutCreateInfo.flags = 0;

			layoutCreateInfo.bindingCount = 1;
			layoutCreateInfo.pBindings = &objectBinding;

			vkCreateDescriptorSetLayout(GraphicsContext::GetDevice()->GetHandle(), &layoutCreateInfo, nullptr, &m_objectSetLayout);

			VulkanDeletionQueue::Push([&]()
				{
					vkDestroyDescriptorSetLayout(GraphicsContext::GetDevice()->GetHandle(), m_objectSetLayout, nullptr);
				});
		}

		// Create descriptor pool
		{
			std::vector<VkDescriptorPoolSize> sizes =
			{
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 }
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

		// Allocate descriptor sets
		{
			m_globalDescriptorSets.resize(framesInFlight);
			m_objectDescriptorSets.resize(framesInFlight);

			for (uint32_t i = 0; i < framesInFlight; i++)
			{
				std::vector<VkWriteDescriptorSet> writeDescriptors;

				// Camera descriptors
				{
					VkDescriptorSetAllocateInfo allocInfo{};
					allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					allocInfo.descriptorPool = m_descriptorPool;
					allocInfo.descriptorSetCount = 1;
					allocInfo.pSetLayouts = &m_globalSetLayout;

					vkAllocateDescriptorSets(GraphicsContext::GetDevice()->GetHandle(), &allocInfo, &m_globalDescriptorSets[i]);

					VkDescriptorBufferInfo bufferInfo{};
					bufferInfo.buffer = m_uniformBufferSet->Get(i)->GetHandle();
					bufferInfo.offset = 0;
					bufferInfo.range = sizeof(CameraData);

					VkWriteDescriptorSet& descriptorWrite = writeDescriptors.emplace_back();
					descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrite.pNext = nullptr;
					descriptorWrite.dstBinding = 0;
					descriptorWrite.dstSet = m_globalDescriptorSets[i];
					descriptorWrite.descriptorCount = 1;
					descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					descriptorWrite.pBufferInfo = &bufferInfo;

				}

				// Object descriptors
				{
					VkDescriptorSetAllocateInfo allocInfo{};
					allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					allocInfo.descriptorPool = m_descriptorPool;
					allocInfo.descriptorSetCount = 1;
					allocInfo.pSetLayouts = &m_objectSetLayout;
					
					vkAllocateDescriptorSets(GraphicsContext::GetDevice()->GetHandle(), &allocInfo, &m_objectDescriptorSets[i]);
					
					VkDescriptorBufferInfo bufferInfo{};
					bufferInfo.buffer = m_objectBufferSet->Get(i)->GetHandle();
					bufferInfo.offset = 0;
					bufferInfo.range = sizeof(ObjectData) * MAX_OBJECT_COUNT;

					VkWriteDescriptorSet& descriptorWrite = writeDescriptors.emplace_back();
					descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrite.pNext = nullptr;
					descriptorWrite.dstBinding = 0;
					descriptorWrite.dstSet = m_objectDescriptorSets[i];
					descriptorWrite.descriptorCount = 1;
					descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					descriptorWrite.pBufferInfo = &bufferInfo;
				}

				vkUpdateDescriptorSets(GraphicsContext::GetDevice()->GetHandle(), (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
			}
		}
	}
}