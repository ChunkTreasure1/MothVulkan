#pragma once

#include "Lamp/Asset/Asset.h"
#include "PipelineCommon.h"

namespace Lamp
{

	class Material;

	class RenderPipeline
	{
	public:
		RenderPipeline(const RenderPipelineSpecification& pipelineSpec);
		RenderPipeline(const RenderPipeline& source);
		RenderPipeline();
		~RenderPipeline();

		void Invalidate();
		void InvalidateMaterials();

		void Bind(VkCommandBuffer cmdBuffer);
		void BindDescriptorSet(VkCommandBuffer cmdBuffer, VkDescriptorSet descriptorSet, uint32_t set, uint32_t passIndex = 0) const;
		void BindDescriptorSets(VkCommandBuffer cmdBuffer, const std::vector<VkDescriptorSet>& descriptorSets, uint32_t firstSet, uint32_t passIndex = 0) const;

		void SetPushConstant(VkCommandBuffer cmdBuffer, uint32_t offset, uint32_t size, const void* data) const;
		void SetShader(Ref<Shader> shader);
		void SetRenderPass(Ref<RenderPass> renderPass);

		void AddReference(Material* material);
		void RemoveReference(Material* material);

		inline const size_t GetHash() const { return m_hash; }
		inline const RenderPipelineSpecification& GetSpecification() const { return m_specification; }

		static Ref<RenderPipeline> Create(const RenderPipelineSpecification& pipelineSpec);
		static Ref<RenderPipeline> Create();

	private:
		void Release();
		void SetVertexLayout();
		void GenerateHash();

		std::vector<VkVertexInputBindingDescription> m_vertexBindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> m_vertexAttributeDescriptions;
		std::vector<Material*> m_materialReferences;

		VkPipelineLayout m_pipelineLayout = nullptr;
		VkPipeline m_pipeline = nullptr;

		size_t m_hash = 0;
		RenderPipelineSpecification m_specification;
	}; 
}