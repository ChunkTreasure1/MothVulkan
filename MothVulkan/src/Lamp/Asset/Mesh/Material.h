#pragma once

#include "Lamp/Asset/Asset.h"

#include "Lamp/Rendering/Shader/Shader.h"

namespace Lamp
{
	class RenderPipeline;
	class Texture2D;

	class Material : public Asset
	{
	public:
		Material() = default;
		Material(const std::string& name, uint32_t index, Ref<RenderPipeline> renderPipeline);
		~Material();

		void Bind(VkCommandBuffer commandBuffer, uint32_t frameIndex) const;
		void SetTexture(uint32_t binding, Ref<Texture2D> texture);
		void Invalidate();

		inline const std::string& GetName() const { return m_name; }

		static AssetType GetStaticType() { return AssetType::Material; }
		AssetType GetType() override { return GetStaticType(); }
		 
		static Ref<Material> Create(const std::string& name, uint32_t index, Ref<RenderPipeline> renderPipeline);

	private:

		void CreateDescriptorPool();
		void AllocateAndSetupDescriptorSets();

		void SetupMaterialFromPipeline();
		void UpdateTextureWriteDescriptor(uint32_t binding);

		Ref<RenderPipeline> m_renderPipeline;

		std::map<uint32_t, Ref<Texture2D>> m_textures; // binding -> texture
		std::vector<Shader::ShaderResources> m_shaderResources;
		std::vector<std::vector<uint32_t>> m_descriptorSetBindings; // maps descriptor set vector index to descriptor set binding
		std::vector<std::vector<VkDescriptorSet>> m_frameDescriptorSets; // frame -> index -> descriptor set
		std::vector<std::vector<VkWriteDescriptorSet>> m_writeDescriptors;

		VkDescriptorPool m_descriptorPool = nullptr;

		std::string m_name;
		uint32_t m_index;
	};
}