#include "lppch.h"
#include "RenderPipeline.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Log/Log.h"

#include "Lamp/Rendering/Vertex.h"

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
		}
	}

	RenderPipeline::RenderPipeline(const RenderPipelineSpecification& pipelineSpec)
		: m_specification(pipelineSpec)
	{
		Invalidate();
	}

	RenderPipeline::~RenderPipeline()
	{
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

	void RenderPipeline::Invalidate()
	{
		SetVertexLayout();

		auto device = GraphicsContext::GetDevice();
		
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
		
		
	}
	
	void RenderPipeline::Release()
	{
	}
}