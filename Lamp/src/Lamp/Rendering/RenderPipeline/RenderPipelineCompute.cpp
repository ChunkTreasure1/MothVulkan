#include "lppch.h"
#include "RenderPipelineCompute.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"

#include "Lamp/Log/Log.h"

namespace Lamp
{
	RenderPipelineCompute::RenderPipelineCompute(Ref<Shader> computeShader)
		: m_shader(computeShader)
	{
		CreatePipeline();
	}

	void RenderPipelineCompute::Begin(VkCommandBuffer commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
	}

	void RenderPipelineCompute::End()
	{
	}

	void RenderPipelineCompute::Dispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
	}

	Ref<RenderPipelineCompute> RenderPipelineCompute::Create(Ref<Shader> computeShader)
	{
		return CreateRef<RenderPipelineCompute>(computeShader);
	}

	void RenderPipelineCompute::CreatePipeline()
	{
		auto device = GraphicsContext::GetDevice();

		m_shaderResources = m_shader->GetResources();

		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = (uint32_t)m_shaderResources.paddedSetLayouts.size();
		layoutInfo.pSetLayouts = m_shaderResources.paddedSetLayouts.data();

		LP_VK_CHECK(vkCreatePipelineLayout(device->GetHandle(), &layoutInfo, nullptr, &m_pipelineLayout));

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = m_pipelineLayout;
		pipelineInfo.flags = 0;
		pipelineInfo.stage = m_shader->GetStageInfos()[0];

		VkPipelineCacheCreateInfo pipelineCacheInfo{};
		pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

		LP_VK_CHECK(vkCreatePipelineCache(device->GetHandle(), &pipelineCacheInfo, nullptr, &m_pipelineCache));
		LP_VK_CHECK(vkCreateComputePipelines(device->GetHandle(), m_pipelineCache, 1, &pipelineInfo, nullptr, &m_pipeline));
	
		CreateDescriptorPool();
	}

	void RenderPipelineCompute::CreateDescriptorPool()
	{
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = 0;
		poolInfo.maxSets = 100;
		poolInfo.poolSizeCount = (uint32_t)m_shaderResources.poolSizes.size();
		poolInfo.pPoolSizes = m_shaderResources.poolSizes.data();

		LP_VK_CHECK(vkCreateDescriptorPool(GraphicsContext::GetDevice()->GetHandle(), &poolInfo, nullptr, &m_descriptorPool));
	}
}