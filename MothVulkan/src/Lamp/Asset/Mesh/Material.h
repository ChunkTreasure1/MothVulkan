#pragma once

#include "Lamp/Asset/Asset.h"

#include "Lamp/Rendering/Shader/Shader.h"

namespace Lamp
{
	class RenderPipeline;
	class Texture2D;
	class MaterialInstance;

	class Material : public Asset
	{
	public:
		Material(const std::string& name, uint32_t index, Ref<RenderPipeline> renderPipeline);

		void SetTexture(uint32_t binding, Ref<Texture2D> texture);

		void AddReference(MaterialInstance* materialInstance);
		void RemoveReference(MaterialInstance* materialInstance);

		static AssetType GetStaticType() { return AssetType::Material; }
		AssetType GetType() override { return GetStaticType(); }
		 
		static Ref<Material> Create(const std::string& name, uint32_t index, Ref<RenderPipeline> renderPipeline);

	private:
		friend class MaterialInstance;

		void SetupMaterialFromPipeline();
		void UpdateTextureWriteDescriptor(uint32_t binding);

		Ref<RenderPipeline> m_renderPipeline;

		std::vector<Shader::ShaderResources> m_shaderResources;
		std::vector<MaterialInstance*> m_materialInstanceReferences;

		std::map<uint32_t, Ref<Texture2D>> m_textures; // binding -> texture

		std::string m_name;
		uint32_t m_index;
	};
}