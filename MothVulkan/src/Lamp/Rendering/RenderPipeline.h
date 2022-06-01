#pragma once

#include "Lamp/Core/Base.h"

#include "Lamp/Rendering/Buffer/BufferLayout.h"

namespace Lamp
{
	class Shader;

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

	struct RenderPipelineSpecification
	{
		Ref<Shader> shader;
		
		Topology topology = Topology::TriangleList;
		CullMode cullMode = CullMode::Back;
		FillMode fillMode = FillMode::Solid;
		DepthMode depthMode = DepthMode::ReadWrite;

		float lineWidth = 1.f;
		uint32_t tessellationControlPoints = 4;

		BufferLayout vertexLayout;
		BufferLayout instanceLayout;
		std::string debugName;
	};

	class RenderPipeline
	{
	public:
		RenderPipeline(const RenderPipelineSpecification& pipelineSpec);
		~RenderPipeline();

		void Invalidate();
		inline const size_t GetHash() const { return m_hash; }

		static Ref<RenderPipeline> Create(const RenderPipelineSpecification& pipelineSpec);

	private:
		void Release();
		void SetVertexLayout();

		std::vector<VkVertexInputBindingDescription> m_vertexBindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> m_vertexAttributeDescriptions;

		size_t m_hash = 0;
		RenderPipelineSpecification m_specification;
	}; 
}