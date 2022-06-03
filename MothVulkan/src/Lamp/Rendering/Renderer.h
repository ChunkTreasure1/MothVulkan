#pragma once

#include "Lamp/Core/Base.h"

#include <vulkan/vulkan.h>

namespace Lamp
{
	class RenderPass;
	class CommandBuffer;
	class Framebuffer;

	class Mesh;
	class Texture2D;
	class Material;
	class MaterialInstance;
	class RenderPipeline;

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

		static void Draw(); // WILL BE REMOVED

		inline static const DefaultData& GetDefaultData() { return *s_defaultData; }

	private:
		Renderer() = delete;
		
		static void CreateDefaultData();

		struct RendererData
		{
			Ref<CommandBuffer> commandBuffer;
			Ref<Framebuffer> currentFramebuffer;
		
			/////TESTING/////
			Ref<Mesh> mesh;
			Ref<Texture2D> texture;

			Ref<Material> material;
			Ref<MaterialInstance> materialInstance;
			Ref<RenderPipeline> renderPipeline;

		};

		inline static Scope<DefaultData> s_defaultData;
		inline static Scope<RendererData> s_rendererData;
	};
}