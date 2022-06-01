#pragma once

#include "Lamp/Asset/Asset.h"

namespace Lamp
{
	class RenderPipeline;
	class Material : public Asset
	{
	public:
		Material(const std::string& name, uint32_t index);

		static AssetType GetStaticType() { return AssetType::Material; }
		AssetType GetType() override { return GetStaticType(); }
		 
		static Ref<Material> Create(const std::string& name, uint32_t index);

	private:
		Ref<RenderPipeline> m_renderPipeline;

		std::string m_name;
		uint32_t m_index;
	};
}