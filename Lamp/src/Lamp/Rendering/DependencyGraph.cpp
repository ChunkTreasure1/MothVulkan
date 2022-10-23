#include "lppch.h"
#include "DependencyGraph.h"

#include "Lamp/Rendering/Texture/ImageBarrier.h"
#include "Lamp/Rendering/Framebuffer.h"

#include "Lamp/Rendering/RenderPass/RenderPassRegistry.h"
#include "Lamp/Rendering/RenderPass/RenderPass.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipelineCompute.h"
#include "Lamp/Rendering/RenderGraph.h"

#include "Lamp/Rendering/RenderPipeline/RenderPipelineRegistry.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipeline.h"
#include "Lamp/Asset/RenderPipelineAsset.h"

#include "Lamp/Utility/ImageUtility.h"

namespace Lamp
{
	DependencyGraph::DependencyGraph(Ref<RenderGraph> renderGraph)
		: m_renderGraph(renderGraph)
	{
		Invalidate();
	}

	void DependencyGraph::InsertBarriersForPass(VkCommandBuffer commandBuffer, size_t passHash)
	{
		auto it = m_toDependencyImageBarriers.find(passHash);
		if (it == m_toDependencyImageBarriers.end())
		{
			return;
		}

		const auto& barriers = m_toDependencyImageBarriers.at(passHash);

		for (const auto& imageBarrier : barriers)
		{
			const auto& barrier = imageBarrier->GetBarrier();
			if (barrier.oldLayout != barrier.newLayout)
			{
				const auto [sourceStage, dstStage] = Utility::GetStageFlagsFromLayouts(barrier.oldLayout, barrier.newLayout);
				vkCmdPipelineBarrier(commandBuffer, sourceStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			}
		}
	}

	Ref<DependencyGraph> DependencyGraph::Create(Ref<RenderGraph> renderGraph)
	{
		return CreateRef<DependencyGraph>(renderGraph);
	}

	void DependencyGraph::Invalidate()
	{
		for (const auto& pass : m_renderGraph->GetRenderPasses())
		{
			// Dependency per pass
			if (pass.renderPass->computePipeline)
			{
				for (size_t i = 0; i < pass.renderPass->framebuffer->GetSpecification().attachments.size(); i++)
				{
					if (Utility::IsDepthFormat(pass.renderPass->framebuffer->GetSpecification().attachments.at(i).format))
					{
						m_toDependencyImageBarriers[pass.renderPass->hash].emplace_back(ImageBarrier::Create(pass.renderPass->framebuffer->GetDepthAttachment(), VK_IMAGE_LAYOUT_GENERAL));
					}
					else
					{
						m_toDependencyImageBarriers[pass.renderPass->hash].emplace_back(ImageBarrier::Create(pass.renderPass->framebuffer->GetColorAttachment(i), VK_IMAGE_LAYOUT_GENERAL));
					}
				}
			}
			else
			{
				for (size_t i = 0; i < pass.renderPass->framebuffer->GetSpecification().attachments.size(); i++)
				{
					if (Utility::IsDepthFormat(pass.renderPass->framebuffer->GetSpecification().attachments.at(i).format))
					{
						m_toDependencyImageBarriers[pass.renderPass->hash].emplace_back(ImageBarrier::Create(pass.renderPass->framebuffer->GetDepthAttachment(), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL));
					}
					else
					{
						m_toDependencyImageBarriers[pass.renderPass->hash].emplace_back(ImageBarrier::Create(pass.renderPass->framebuffer->GetColorAttachment(i), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
					}
				}
			}

			// Dependency per pipeline
			for (const auto& [name, pipeline] : RenderPipelineRegistry::GetAllPipelines())
			{
				switch (pipeline->GetPipelineType())
				{
					case PipelineType::Compute:
					{
						if (pipeline->GetComputePipeline()->GetRenderPass() != pass.renderPass->name)
						{
							continue;
						}

						for (const auto& input : pipeline->GetComputePipeline()->GetFramebufferInputs())
						{
							m_toDependencyImageBarriers[pass.renderPass->hash].emplace_back(ImageBarrier::Create(input.framebuffer->GetColorAttachment(input.attachmentIndex), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
						}

						break;
					}

					case PipelineType::Graphics:
					{
						if (pipeline->GetGraphicsPipeline()->GetSpecification().renderPass != pass.renderPass->name)
						{
							continue;
						}

						for (const auto& input : pipeline->GetGraphicsPipeline()->GetSpecification().framebufferInputs)
						{
							m_toDependencyImageBarriers[pass.renderPass->hash].emplace_back(ImageBarrier::Create(input.framebuffer->GetColorAttachment(input.attachmentIndex), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
						}

						break;
					}
				}
			}
		}
	}
}