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

	void DependencyGraph::InsertBarriersPrePass(VkCommandBuffer commandBuffer, size_t passHash)
	{
		auto it = m_preDependencyImageBarriers.find(passHash);
		if (it == m_preDependencyImageBarriers.end())
		{
			return;
		}

		const auto& barriers = m_preDependencyImageBarriers.at(passHash);

		for (const auto& imageBarrier : barriers)
		{
			const auto& barrier = imageBarrier->GetBarrier();
			if (barrier.oldLayout != barrier.newLayout)
			{
				const auto [sourceStage, dstStage] = Utility::GetStageFlagsFromLayouts(barrier.oldLayout, barrier.newLayout);
				vkCmdPipelineBarrier(commandBuffer, sourceStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

				imageBarrier->UpdateLayout(barrier.newLayout);
			}
		}
	}

	void DependencyGraph::InsertBarriersPostPass(VkCommandBuffer commandBuffer, size_t passHash)
	{
		auto it = m_postDependencyImageBarriers.find(passHash);
		if (it == m_postDependencyImageBarriers.end())
		{
			return;
		}

		const auto& barriers = m_postDependencyImageBarriers.at(passHash);

		for (const auto& imageBarrier : barriers)
		{
			const auto& barrier = imageBarrier->GetBarrier();
			if (barrier.oldLayout != barrier.newLayout)
			{
				const auto [sourceStage, dstStage] = Utility::GetStageFlagsFromLayouts(barrier.oldLayout, barrier.newLayout);
				vkCmdPipelineBarrier(commandBuffer, sourceStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

				imageBarrier->UpdateLayout(barrier.newLayout);
			}
		}
	}

	Ref<DependencyGraph> DependencyGraph::Create(Ref<RenderGraph> renderGraph)
	{
		return CreateRef<DependencyGraph>(renderGraph);
	}

	void DependencyGraph::Invalidate()
	{
		for (size_t index = 0; const auto & pass : m_renderGraph->GetRenderPasses())
		{
			// Dependency per pass, also add shader read only to final pass
			index++;

			if (pass.renderPass->computePipeline)
			{
				for (uint32_t i = 0; i < (uint32_t)pass.renderPass->framebuffer->GetSpecification().attachments.size(); i++)
				{
					if (Utility::IsDepthFormat(pass.renderPass->framebuffer->GetSpecification().attachments.at(i).format))
					{
						m_preDependencyImageBarriers[pass.renderPass->hash].emplace_back(ImageBarrier::Create(pass.renderPass->framebuffer->GetDepthAttachment(), VK_IMAGE_LAYOUT_GENERAL));
					
						if (index == m_renderGraph->GetRenderPasses().size())
						{
							m_postDependencyImageBarriers[pass.renderPass->hash].emplace_back(ImageBarrier::Create(pass.renderPass->framebuffer->GetDepthAttachment(), VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL));
						}
					}
					else
					{
						m_preDependencyImageBarriers[pass.renderPass->hash].emplace_back(ImageBarrier::Create(pass.renderPass->framebuffer->GetColorAttachment(i), VK_IMAGE_LAYOUT_GENERAL));

						if (index == m_renderGraph->GetRenderPasses().size())
						{
							m_postDependencyImageBarriers[pass.renderPass->hash].emplace_back(ImageBarrier::Create(pass.renderPass->framebuffer->GetColorAttachment(i), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
						}
					}
				}
			}
			else
			{
				for (uint32_t i = 0; i < (uint32_t)pass.renderPass->framebuffer->GetSpecification().attachments.size(); i++)
				{
					if (Utility::IsDepthFormat(pass.renderPass->framebuffer->GetSpecification().attachments.at(i).format))
					{
						m_preDependencyImageBarriers[pass.renderPass->hash].emplace_back(ImageBarrier::Create(pass.renderPass->framebuffer->GetDepthAttachment(), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL));

						if (index == m_renderGraph->GetRenderPasses().size())
						{
							m_postDependencyImageBarriers[pass.renderPass->hash].emplace_back(ImageBarrier::Create(pass.renderPass->framebuffer->GetDepthAttachment(), VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL));
						}
					}
					else
					{
						m_preDependencyImageBarriers[pass.renderPass->hash].emplace_back(ImageBarrier::Create(pass.renderPass->framebuffer->GetColorAttachment(i), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));

						if (index == m_renderGraph->GetRenderPasses().size())
						{
							m_postDependencyImageBarriers[pass.renderPass->hash].emplace_back(ImageBarrier::Create(pass.renderPass->framebuffer->GetColorAttachment(i), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
						}
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
							m_preDependencyImageBarriers[pass.renderPass->hash].emplace_back(ImageBarrier::Create(input.framebuffer->GetColorAttachment(input.attachmentIndex), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
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
							m_preDependencyImageBarriers[pass.renderPass->hash].emplace_back(ImageBarrier::Create(input.framebuffer->GetColorAttachment(input.attachmentIndex), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
						}

						break;
					}
				}
			}
		}
	}
}