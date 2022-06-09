#pragma once

#include "Lamp/Asset/Asset.h"

namespace Lamp
{
	class RenderPass;
	class RenderGraph : public Asset
	{
	public:
		struct RenderPassContainer
		{
			Ref<RenderPass> renderPass;
			int32_t priority = 0;
		};

		RenderGraph() = default;

		inline const std::string& GetName() const { return m_name; }
		inline const std::vector<RenderPassContainer>& GetRenderPasses() { return m_renderPasses; };
		
		static AssetType GetStaticType() { return AssetType::RenderGraph; }
		AssetType GetType() override { return GetStaticType(); }

	private:
		friend class RenderGraphImporter;

		std::vector<RenderPassContainer> m_renderPasses;
		std::string m_name;
	};
}