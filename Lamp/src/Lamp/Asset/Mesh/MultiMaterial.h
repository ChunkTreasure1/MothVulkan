#pragma once

#include "Lamp/Asset/Asset.h"

namespace Lamp
{
	class Material;
	class MultiMaterial : public Asset
	{
	public:
		MultiMaterial() = default;

		inline const std::unordered_map<uint32_t, Ref<Material>>& GetMaterials() const { return m_materials; }
		inline const std::string& GetName() const { return m_name; }

		static AssetType GetStaticType() { return AssetType::Material; }
		AssetType GetType() override { return GetStaticType(); }
		
	private:
		friend class FbxImporter;
		friend class GLTFImporter;
		friend class MultiMaterialImporter;
		friend class MeshCompiler;

		std::string m_name;
		std::unordered_map<uint32_t, Ref<Material>>	m_materials;
	};
}