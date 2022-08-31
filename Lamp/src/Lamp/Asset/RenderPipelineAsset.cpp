#include "lppch.h"
#include "RenderPipelineAsset.h"

#include "Lamp/Rendering/RenderPipeline/RenderPipelineCompute.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipeline.h"
#include "Lamp/Rendering/Framebuffer.h"

#include "Lamp/Rendering/Buffer/UniformBuffer/UniformBufferRegistry.h"
#include "Lamp/Rendering/Buffer/ShaderStorageBuffer/ShaderStorageBufferRegistry.h"

#include "Lamp/Core/Application.h"
#include "Lamp/Core/Window.h"
#include "Lamp/Core/Graphics/Swapchain.h"

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

		case PipelineType::Compute:
		{
			SetupComputePipeline(pipelineSpec);
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

	void RenderPipelineAsset::SetupComputePipeline(const RenderPipelineSpecification& spec)
	{
		const uint32_t framesInFlight = Application::Get().GetWindow()->GetSwapchain().GetFramesInFlight();

		Ref<RenderPipelineCompute> computePipeline = RenderPipelineCompute::Create(spec.shader, framesInFlight);

		auto shaderResources = spec.shader->GetResources();
		for (auto& [set, bindings] : shaderResources.uniformBuffersInfos)
		{
			for (auto& [binding, info] : bindings)
			{
				computePipeline->SetUniformBuffer(UniformBufferRegistry::Get(set, binding), set, binding);
			}
		}

		for (auto& [set, bindings] : shaderResources.storageBuffersInfos)
		{
			for (auto& [binding, info] : bindings)
			{
				computePipeline->SetStorageBuffer(ShaderStorageBufferRegistry::Get(set, binding), set, binding);
			}
		}

		m_computePipeline = computePipeline;
		m_computePipeline->m_framebufferInputs = spec.framebufferInputs;
	}
}