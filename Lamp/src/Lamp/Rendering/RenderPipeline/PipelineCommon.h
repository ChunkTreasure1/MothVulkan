#pragma once

#include "Lamp/Core/Base.h"
#include "Lamp/Rendering/Buffer/BufferLayout.h"

namespace Lamp
{
	class Framebuffer;
	class RenderPass;
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

	enum class PipelineType : uint32_t
	{
		Graphics,
		Compute
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
		PipelineType type = PipelineType::Graphics;

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
}