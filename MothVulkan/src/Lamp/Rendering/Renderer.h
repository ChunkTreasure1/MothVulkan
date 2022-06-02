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
		static void Initialize();
		static void Shutdowm();

		static void Begin();
		static void End();

		static void BeginPass(Ref<RenderPass> renderPass);
		static void EndPass();

		static void Draw(); // WILL BE REMOVED

	private:
		Renderer() = delete;

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

		inline static Scope<RendererData> s_rendererData;
	};
}