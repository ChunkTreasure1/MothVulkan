#pragma once

#include "Lamp/Core/Base.h"

#include "Lamp/Asset/Asset.h"

namespace Lamp
{
	class Framebuffer;
	class RenderPipeline;
	class RenderPipelineCompute;

	enum class DrawType : uint32_t
	{
		FullscreenQuad = 0,
		Opaque
	};

	class RenderPass : public Asset
	{
	public:
		std::string name;
		Ref<Framebuffer> framebuffer;
		
		Ref<RenderPipeline> overridePipeline;
		std::string overridePipelineName;
		
		Ref<RenderPipelineCompute> computePipeline;
		std::string computePipelineName;

		std::string exclusivePipelineName;
		size_t exclusivePipelineHash = 0;

		std::vector<std::string> excludedPipelineNames;
		std::vector<size_t> excludedPipelineHashes;

		DrawType drawType = DrawType::Opaque;
		int32_t priority = 0;
		bool resizeable = true;

		static AssetType GetStaticType() { return AssetType::RenderPass; }
		AssetType GetType() override { return GetStaticType(); }
	};
}