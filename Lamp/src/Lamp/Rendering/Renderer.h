#pragma once

#include "Lamp/Core/Base.h"
#include "Lamp/Asset/Mesh/SubMesh.h"

#include <vulkan/vulkan.h>
#include <functional>

namespace Lamp
{
	class RenderPass;
	class CommandBuffer;
	class Framebuffer;
	class ShaderStorageBufferSet;

	class Mesh;
	class Texture2D;
	class Material;
	class RenderPipeline;

	struct RenderCommand
	{
		Ref<Mesh> mesh;
		Ref<Material> material;
		SubMesh subMesh;
		glm::mat4 transform;
	};

	struct IndirectBatch
	{
		Ref<Mesh> mesh;
		Ref<Material> material;
		SubMesh subMesh;
		glm::mat4 transform;
		uint32_t first;
		uint32_t count;
	};

	class Renderer
	{
	public:
		struct DefaultData
		{
			Ref<Texture2D> whiteTexture;
			Ref<Texture2D> blackCubeTexture;
		};

		struct Capabilities
		{};

		static void Initialize();
		static void Shutdowm();

		static void Begin();
		static void End();

		static void BeginPass(Ref<RenderPass> renderPass);
		static void EndPass();

		static void Submit(Ref<Mesh> mesh, const glm::mat4& transform);

		static void Draw(); // WILL BE REMOVED
		static void TEST_RecompileShader();

		inline static const DefaultData& GetDefaultData() { return *s_defaultData; }

	private:
		Renderer() = delete;
		
		static void CreateDefaultData();
		static std::vector<IndirectBatch> PrepareForIndirectDraw(const std::vector<RenderCommand>& renderCommands);

		struct RendererData
		{
			Ref<CommandBuffer> commandBuffer;
			Ref<Framebuffer> currentFramebuffer;

			Ref<ShaderStorageBufferSet> indirectDrawBuffer;
			std::vector<RenderCommand> renderCommands;

			/////TESTING/////
			Ref<Mesh> mesh;
			Ref<Texture2D> texture;

			Ref<Material> material;
			Ref<RenderPipeline> renderPipeline;
		};

		inline static Scope<DefaultData> s_defaultData;
		inline static Scope<RendererData> s_rendererData;
	};
}