#pragma once

#include "Lamp/Core/Base.h"
#include "Lamp/Asset/Mesh/SubMesh.h"

#include "Lamp/Rendering/DeletionQueue.hpp"

#include <vulkan/vulkan.h>
#include <functional>

namespace Lamp
{
	class RenderPass;
	class CommandBuffer;
	class Framebuffer;
	class ShaderStorageBufferSet;
	class Camera;

	class Mesh;
	class Texture2D;
	class Material;
	class RenderPipelineCompute;

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
		uint32_t first;
		uint32_t count;
		uint32_t id;
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
		static void InitializeBuffers();

		static void Shutdowm();

		static void Begin();
		static void End();

		static void BeginPass(Ref<RenderPass> renderPass, Ref<Camera> camera);
		static void EndPass();

		static void Submit(Ref<Mesh> mesh, const glm::mat4& transform);

		static void Draw(); // WILL BE REMOVED

		static void SubmitDestroy(std::function<void()>&& function);

		static VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetAllocateInfo& allocInfo);
		inline static const DefaultData& GetDefaultData() { return *s_defaultData; }

	private:
		Renderer() = delete;
		
		static void CreateDefaultData();
		static void CreateDescriptorPools();
		static std::vector<IndirectBatch> PrepareForIndirectDraw(std::vector<RenderCommand>& renderCommands);

		static void UpdatePerPassBuffers();
		static void UpdatePerFrameBuffers();

		static void SortRenderCommands();
		static void UploadAndCullRenderCommands();

		struct RendererData
		{
			std::vector<DeletionQueue> frameDeletionQueues;

			Ref<CommandBuffer> commandBuffer;
			Ref<Framebuffer> currentFramebuffer;

			Ref<ShaderStorageBufferSet> indirectDrawBuffer;
			Ref<ShaderStorageBufferSet> indirectCountBuffer;
			Ref<ShaderStorageBufferSet> objectBuffer;

			Ref<RenderPipelineCompute> indirectCullPipeline;

			std::vector<RenderCommand> renderCommands;
			Ref<Camera> passCamera;

			std::vector<VkDescriptorPool> descriptorPools;
		};

		inline static Scope<DefaultData> s_defaultData;
		inline static Scope<RendererData> s_rendererData;
	};
}