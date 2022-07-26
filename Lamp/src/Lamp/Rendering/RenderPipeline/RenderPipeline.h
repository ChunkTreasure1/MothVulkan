#pragma once

#include "Lamp/Core/Base.h"
#include "Lamp/Asset/Asset.h"

#include "Lamp/Rendering/Buffer/BufferLayout.h"

namespace Lamp
{
	class Shader;
	class Framebuffer;
	class RenderPass;
	class Material;

	enum class Topology : uint32_t
	{
		TriangleList = 0,
		LineList,
		TriangleStrip,
		PatchList
	};

	enum class CullMode : uint32_t
	{
		Front = 0,
		Back,
		FrontAndBack,
		None
	};

	enum class FillMode : uint32_t
	{
		Solid = 0,
		Wireframe
	};

	enum class DepthMode : uint32_t
	{
		Read = 0,
		Write,
		ReadWrite,
		None
	};

	struct FramebufferInput
	{
		Ref<Framebuffer> framebuffer;
		std::string renderPass;
		uint32_t attachmentIndex = 0;
		uint32_t set = 0;
		uint32_t binding = 0;
	};

	struct RenderPipelineSpecification
	{
		Ref<Framebuffer> framebuffer;
		Ref<Shader> shader;
		
		Topology topology = Topology::TriangleList;
		CullMode cullMode = CullMode::Back;
		FillMode fillMode = FillMode::Solid;
		DepthMode depthMode = DepthMode::ReadWrite;

		bool depthTest = true;
		bool depthWrite = true;
		float lineWidth = 1.f;
		uint32_t tessellationControlPoints = 4;

		BufferLayout vertexLayout;
		BufferLayout instanceLayout;
		std::string name;
		std::string renderPass;

		std::vector<FramebufferInput> framebufferInputs;
	};

	class RenderPipeline : public Asset
	{
	public:
		RenderPipeline(const RenderPipelineSpecification& pipelineSpec);
		RenderPipeline(const RenderPipeline& source);
		RenderPipeline();
		~RenderPipeline();

		void Invalidate();
		void InvalidateMaterials();

		void Bind(VkCommandBuffer cmdBuffer);
		void BindDescriptorSet(VkCommandBuffer cmdBuffer, VkDescriptorSet descriptorSet, uint32_t set) const;
		void BindDescriptorSets(VkCommandBuffer cmdBuffer, const std::vector<VkDescriptorSet>& descriptorSets, uint32_t set) const;

		void SetPushConstant(VkCommandBuffer cmdBuffer, uint32_t offset, uint32_t size, const void* data) const;
		void SetShader(Ref<Shader> shader);
		void SetRenderPass(Ref<RenderPass> renderPass);

		void AddReference(Material* material);
		void RemoveReference(Material* material);

		inline const size_t GetHash() const { return m_hash; }
		inline const RenderPipelineSpecification& GetSpecification() const { return m_specification; }

		static AssetType GetStaticType() { return AssetType::RenderPipeline; }
		AssetType GetType() override { return GetStaticType(); }

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