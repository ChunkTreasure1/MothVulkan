#include "lppch.h"
#include "RenderPipelineAsset.h"

#include "Lamp/Rendering/RenderPipeline/RenderPipelineCompute.h"

namespace Lamp
{
	RenderPipelineAsset::RenderPipelineAsset(const RenderPipelineSpecification& pipelineSpec)
		: m_type(pipelineSpec.type), m_name(pipelineSpec.name)
	{
		switch (pipelineSpec.type)
		{
			case PipelineType::Graphics:
			{
				m_renderPipeline = RenderPipeline::Create(pipelineSpec);
				break;
			}
		}
	}

	RenderPipelineAsset::RenderPipelineAsset(const RenderPipelineAsset& asset)
		: m_type(asset.m_type), m_name(asset.m_name)
	{
		switch (asset.m_type)
		{
			case PipelineType::Graphics:
			{
				m_renderPipeline = CreateRef<RenderPipeline>(*asset.m_renderPipeline);
				break;
			}
		}
	}
}