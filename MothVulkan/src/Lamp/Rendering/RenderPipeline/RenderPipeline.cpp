#include "lppch.h"
#include "RenderPipeline.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Log/Log.h"
#include "Lamp/Asset/Mesh/Material.h"

#include "Lamp/Rendering/Vertex.h"
#include "Lamp/Rendering/Framebuffer.h"
#include "Lamp/Rendering/Shader/Shader.h"
#include "Lamp/Rendering/Shader/ShaderUtility.h"

#include "Lamp/Utility/ImageUtility.h"

namespace Lamp
{
	namespace Utility
	{
		static VkPrimitiveTopology LampToVulkanTopology(Topology topology)
		{
			switch (topology)
			{
				case Topology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				case Topology::LineList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				case Topology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
				case Topology::PatchList: return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
			}

			LP_CORE_ASSERT(false, "Topology not supported!");
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}

		static VkPolygonMode LampToVulkanFill(FillMode fillMode)
		{
			switch (fillMode)
			{
				case FillMode::Solid: return VK_POLYGON_MODE_FILL;
				case FillMode::Wireframe: return VK_POLYGON_MODE_LINE;
			}

			LP_CORE_ASSERT(false, "Fill mode not supported!");
			return VK_POLYGON_MODE_FILL;
		}

		static VkCullModeFlags LampToVulkanCull(CullMode cullMode)
		{
			switch (cullMode)
			{
				case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
				case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
				case CullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
				case CullMode::None: return VK_CULL_MODE_NONE;
			}

			LP_CORE_ASSERT(false, "Cull mode not supported!");
			return VK_CULL_MODE_BACK_BIT;
		}
	}

	RenderPipeline::RenderPipeline(const RenderPipelineSpecification& pipelineSpec)
		: m_specification(pipelineSpec)
	{
		LP_CORE_ASSERT(m_specification.framebuffer, "Framebuffer must be specified!");
		LP_CORE_ASSERT(m_specification.shader, "Shader must be specified!");

		m_specification.shader->AddReference(this);

		Invalidate();
		GenerateHash();
	}

	RenderPipeline::~RenderPipeline()
	{
		Release();

		if (m_specification.shader)
		{
			m_specification.shader->RemoveReference(this);
		}
		
		m_materialReferences.clear();
	}

	Ref<RenderPipeline> RenderPipeline::Create(const RenderPipelineSpecification& pipelineSpec)
	{
		return CreateRef<RenderPipeline>(pipelineSpec);
	}

	void RenderPipeline::SetVertexLayout()
	{
		if (m_specification.vertexLayout.GetElements().empty())
		{
			LP_CORE_ERROR("RenderPipeline does not have a vertex layout set! This is required!");
			return;
		}

		auto device = GraphicsContext::GetDevice();

		VkVertexInputBindingDescription& bindingDesc = m_vertexBindingDescriptions.emplace_back();
		bindingDesc.binding = 0;
		bindingDesc.stride = m_specification.vertexLayout.GetStride();
		bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		uint32_t numAttributes = 0;

		for (const auto& attr : m_specification.vertexLayout.GetElements())
		{
			VkVertexInputAttributeDescription& desc = m_vertexAttributeDescriptions.emplace_back();
			desc.binding = 0;
			desc.location = numAttributes;
			desc.format = LampToVulkanFormat(attr.type);
			desc.offset = (uint32_t)attr.offset;

			numAttributes++;
		}

		if (!m_specification.instanceLayout.GetElements().empty())
		{
			VkVertexInputBindingDescription& instanceDesc = m_vertexBindingDescriptions.emplace_back();
			instanceDesc.binding = 1;
			instanceDesc.stride = m_specification.instanceLayout.GetStride();
			instanceDesc.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

			for (const auto& attr : m_specification.instanceLayout.GetElements())
			{
				VkVertexInputAttributeDescription& desc = m_vertexAttributeDescriptions.emplace_back();
				desc.binding = 1;
				desc.location = numAttributes;
				desc.format = LampToVulkanFormat(attr.type);
				desc.offset = (uint32_t)attr.offset;

				numAttributes++;
			}
		}
	}

	void RenderPipeline::GenerateHash()
	{
		size_t hash = m_specification.shader->GetHash();
		hash = Utility::HashCombine(hash, std::hash<uint32_t>()((uint32_t)m_specification.topology));
		hash = Utility::HashCombine(hash, std::hash<uint32_t>()((uint32_t)m_specification.cullMode));
		hash = Utility::HashCombine(hash, std::hash<uint32_t>()((uint32_t)m_specification.fillMode));
		hash = Utility::HashCombine(hash, std::hash<uint32_t>()((uint32_t)m_specification.depthMode));
		hash = Utility::HashCombine(hash, std::hash<std::string>()(m_specification.name));

		m_hash = hash;
	}

	void RenderPipeline::Invalidate()
	{
		Release();

		m_vertexAttributeDescriptions.clear();
		m_vertexBindingDescriptions.clear();

		SetVertexLayout();

		auto device = GraphicsContext::GetDevice();

		// Pipeline layout
		{
			const auto& resources = m_specification.shader->GetResources();

			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = (uint32_t)resources.paddedSetLayouts.size();
			pipelineLayoutInfo.pSetLayouts = resources.paddedSetLayouts.data();
			pipelineLayoutInfo.pushConstantRangeCount = (uint32_t)resources.pushConstantRanges.size();
			pipelineLayoutInfo.pPushConstantRanges = resources.pushConstantRanges.data();

			LP_VK_CHECK(vkCreatePipelineLayout(device->GetHandle(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout));
		}

		// Pipeline
		{
			VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)m_vertexBindingDescriptions.size();
			vertexInputInfo.pVertexBindingDescriptions = m_vertexBindingDescriptions.data();

			vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)m_vertexAttributeDescriptions.size();
			vertexInputInfo.pVertexAttributeDescriptions = m_vertexAttributeDescriptions.data();

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
			inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyInfo.topology = Utility::LampToVulkanTopology(m_specification.topology);
			inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportState{};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.pViewports = nullptr;
			viewportState.scissorCount = 1;
			viewportState.pScissors = nullptr;

			VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
			rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizerInfo.depthBiasClamp = VK_FALSE;
			rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
			rasterizerInfo.polygonMode = Utility::LampToVulkanFill(m_specification.fillMode);
			rasterizerInfo.lineWidth = m_specification.lineWidth;
			rasterizerInfo.cullMode = Utility::LampToVulkanCull(m_specification.cullMode);
			rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizerInfo.depthBiasEnable = VK_FALSE;

			VkPipelineTessellationStateCreateInfo tessellationInfo{};
			tessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
			tessellationInfo.patchControlPoints = m_specification.tessellationControlPoints;

			VkPipelineMultisampleStateCreateInfo multisampleInfo{};
			multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleInfo.sampleShadingEnable = VK_FALSE;
			multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			VkPipelineColorBlendStateCreateInfo blendInfo{};
			blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			blendInfo.logicOpEnable = VK_FALSE;
			blendInfo.logicOp = VK_LOGIC_OP_COPY;
			blendInfo.blendConstants[0] = 0.0f;
			blendInfo.blendConstants[1] = 0.0f;
			blendInfo.blendConstants[2] = 0.0f;
			blendInfo.blendConstants[3] = 0.0f;

			std::vector<VkPipelineColorBlendAttachmentState> blendAttachments;
			for (const auto& attachment : m_specification.framebuffer->GetSpecification().attachments)
			{
				if (Utility::IsDepthFormat(attachment.format))
				{
					continue;
				}

				VkPipelineColorBlendAttachmentState& colorBlendAttachment = blendAttachments.emplace_back();
				colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				colorBlendAttachment.blendEnable = attachment.blendMode != TextureBlend::None ? VK_TRUE : VK_FALSE;

				//TODO: setup correct blendning
			}

			blendInfo.attachmentCount = (uint32_t)blendAttachments.size();
			blendInfo.pAttachments = blendAttachments.data();

			VkPipelineDepthStencilStateCreateInfo depthStencil{};
			depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencil.depthTestEnable = m_specification.depthTest ? VK_TRUE : VK_FALSE;
			depthStencil.depthWriteEnable = m_specification.depthWrite ? VK_TRUE : VK_FALSE;
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencil.stencilTestEnable = VK_FALSE;
			depthStencil.depthBoundsTestEnable = VK_FALSE;

			VkPipelineDynamicStateCreateInfo dynamicInfo{};
			dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicInfo.dynamicStateCount = 2;

			VkDynamicState states[] =
			{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};
			dynamicInfo.pDynamicStates = states;

			VkGraphicsPipelineCreateInfo pipelineInfo{};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.stageCount = (uint32_t)m_specification.shader->GetStageInfos().size();
			pipelineInfo.pStages = m_specification.shader->GetStageInfos().data();
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizerInfo;
			pipelineInfo.pMultisampleState = &multisampleInfo;
			pipelineInfo.pColorBlendState = &blendInfo;
			pipelineInfo.layout = m_pipelineLayout;
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineInfo.pDepthStencilState = &depthStencil;
			pipelineInfo.pDynamicState = &dynamicInfo;
			pipelineInfo.pTessellationState = m_specification.topology == Topology::PatchList ? &tessellationInfo : nullptr;

			VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
			pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
			pipelineRenderingInfo.colorAttachmentCount = (uint32_t)m_specification.framebuffer->m_colorAttachmentInfos.size();
			pipelineRenderingInfo.pColorAttachmentFormats = m_specification.framebuffer->m_colorFormats.data();

			if (m_specification.framebuffer->GetDepthAttachment())
			{
				pipelineRenderingInfo.depthAttachmentFormat = m_specification.framebuffer->m_depthFormat;
				if (m_specification.framebuffer->m_depthFormat == VK_FORMAT_D24_UNORM_S8_UINT)
				{
					pipelineRenderingInfo.stencilAttachmentFormat = m_specification.framebuffer->m_depthFormat;
				}
				else
				{
					pipelineRenderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
				}
			}
			else
			{
				pipelineRenderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
				pipelineRenderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
			}

			pipelineInfo.pNext = &pipelineRenderingInfo;
			LP_VK_CHECK(vkCreateGraphicsPipelines(device->GetHandle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline));
		}

		for (const auto& mat : m_materialReferences)
		{
			mat->Invalidate();
		}
	}

	void RenderPipeline::Bind(VkCommandBuffer cmdBuffer)
	{
		VkExtent2D extent{};
		extent.width = m_specification.framebuffer->GetWidth();
		extent.height = m_specification.framebuffer->GetHeight();

		VkViewport viewport{};
		viewport.x = 0.f;
		viewport.y = (float)extent.height;

		viewport.width = (float)extent.width;
		viewport.height = -(float)extent.height;
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		VkRect2D scissor = { { 0, 0 }, extent };
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
	}

	void RenderPipeline::BindDescriptorSet(VkCommandBuffer cmdBuffer, VkDescriptorSet descriptorSet, uint32_t set) const
	{
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, set, 1, &descriptorSet, 0, nullptr);
	}

	void RenderPipeline::AddReference(Material* material)
	{
		if (auto it = std::find(m_materialReferences.begin(), m_materialReferences.end(), material); it != m_materialReferences.end())
		{
			LP_CORE_ERROR("Render pipeline {0} already has a reference to material {1}!", m_specification.name.c_str(), material->GetName().c_str());
			return;
		}

		m_materialReferences.emplace_back(material);
	}

	void RenderPipeline::RemoveReference(Material* material)
	{
		auto it = std::find(m_materialReferences.begin(), m_materialReferences.end(), material);
		if (it == m_materialReferences.end())
		{
			LP_CORE_ERROR("Material reference to material {0} was not found in render pipeline {1}!", material->GetName().c_str(), m_specification.name.c_str());
			return;
		}

		m_materialReferences.erase(it);
	}

	void RenderPipeline::Release()
	{
		if (m_pipeline != VK_NULL_HANDLE)
		{
			auto device = GraphicsContext::GetDevice();
			vkDestroyPipelineLayout(device->GetHandle(), m_pipelineLayout, nullptr);
			vkDestroyPipeline(device->GetHandle(), m_pipeline, nullptr);
		}
	}
}