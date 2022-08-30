#pragma once

#include "Lamp/Core/Base.h"
#include "Lamp/Asset/Asset.h"

#include "Lamp/Rendering/Buffer/BufferLayout.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipeline.h"

namespace Lamp
{
	class Shader;
	class Framebuffer;
	class RenderPass;
	class Material;

	class RenderPipeline;
	class RenderPipelineCompute;

	struct RenderPipelineSpecification;

	class RenderPipelineAsset : public Asset
	{
	public:
		RenderPipelineAsset() = default;
		RenderPipelineAsset(const RenderPipelineSpecification& pipelineSpec);
		RenderPipelineAsset(const RenderPipelineAsset& asset);

		static AssetType GetStaticType() { return AssetType::RenderPipeline; }
		AssetType GetType() override { return GetStaticType(); }

		inline const std::string& GetName() const { return m_name; }
		inline const PipelineType& GetPipelineType() const { return m_type; }

		inline const Ref<RenderPipeline> GetGraphicsPipeline() const { return m_renderPipeline; }
		inline const Ref<RenderPipelineCompute> GetComputePipeline() const { return m_computePipeline; }

	private:
		void SetupComputePipeline(const RenderPipelineSpecification& spec);

		friend class RenderPipelineImporter;

		std::string m_name = "None";
		PipelineType m_type = PipelineType::Graphics;

		Ref<RenderPipeline> m_renderPipeline;
		Ref<RenderPipelineCompute> m_computePipeline;
	};
}