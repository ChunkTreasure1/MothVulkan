#pragma once

#include "Lamp/Core/Base.h"

#include "Lamp/Asset/Asset.h"

namespace Lamp
{
	class Framebuffer;
	class RenderPipeline;

	enum class DrawType
	{
		FullscreenQuad,
		Opaque
	};

	class RenderPass : public Asset
	{
	public:
		std::string name;
		Ref<Framebuffer> framebuffer;
		
		Ref<RenderPipeline> overridePipeline;
		std::string overridePipelineName;

		DrawType drawType = DrawType::Opaque;

		static AssetType GetStaticType() { return AssetType::RenderPass; }
		AssetType GetType() override { return GetStaticType(); }
	};
}