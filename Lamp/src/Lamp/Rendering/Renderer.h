#pragma once

#include "Lamp/Core/Base.h"
#include "Lamp/Asset/Mesh/SubMesh.h"
#include "Lamp/Asset/Asset.h"

#include "Lamp/Rendering/FunctionQueue.hpp"
#include "Lamp/Rendering/RendererStructs.h"

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
	class MultiMaterial;
	class Texture2D;
	class Material;
	class RenderPipelineCompute;
	class RenderPipeline;
	
	class CombinedIndexBuffer;
	class CombinedVertexBuffer;
	class TextureTable;
	class MaterialTable;

	class DependencyGraph;

	struct RenderCommand
	{
		Ref<Mesh> mesh;
		Ref<Material> material;
		SubMesh subMesh;
		glm::mat4 transform;

		uint32_t firstInstance = 0;
		uint32_t batchId = 0;
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
			Ref<RenderPipeline> defaultPipeline;
			Ref<MultiMaterial> defaultMaterial;

			Ref<Texture2D> whiteTexture;
			Ref<Image2D> blackCubeImage;
			Ref<Image2D> brdfLut;
		};

		struct Capabilities
		{};

		struct BindlessData
		{
			inline static constexpr uint32_t MAX_TRIANGLES = 10000000;
			inline static constexpr uint32_t MAX_VERTICES = MAX_TRIANGLES * 3;
			inline static constexpr uint32_t MAX_INDICES = MAX_TRIANGLES * 3;

			Ref<CombinedVertexBuffer> combinedVertexBuffer;
			Ref<CombinedIndexBuffer> combinedIndexBuffer;

			Ref<TextureTable> textureTable;
			Ref<MaterialTable> materialTable;

			VkDescriptorPool descriptorPool = nullptr;
			VkDescriptorSetLayout descriptorSetLayout = nullptr;
			VkDescriptorSet descriptorSet = nullptr;
		};

		static void Initialize();
		static void InitializeBuffers();

		static void Shutdowm();

		static void Begin();
		static void End();

		static void ExecuteComputePass();

		static void BeginPass(Ref<RenderPass> renderPass, Ref<Camera> camera);
		static void EndPass();

		static void Submit(Ref<Mesh> mesh, const glm::mat4& transform);
		static void SubmitDirectionalLight(const glm::mat4& transform, const glm::vec3& color, const float intensity);
		static void SubmitEnvironment(const Skybox& environment);

		static void DispatchRenderCommands();
		static void DispatchRenderCommandsTest();

		static void SubmitResourceFree(std::function<void()>&& function);
		static void SubmitInvalidation(std::function<void()>&& function);

		inline static BindlessData& GetBindlessData() { return s_rendererData->bindlessData; }

		static Skybox GenerateEnvironmentMap(AssetHandle handle);

		static VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetAllocateInfo& allocInfo);
		inline static const DefaultData& GetDefaultData() { return *s_defaultData; }

	private:
		Renderer() = delete;
		
		static void CreateDefaultData();
		static void CreateSamplers();
		static void CreateDescriptorPools();
		static void CreateBindlessData();

		static void UpdatePerPassBuffers();
		static void UpdatePerFrameBuffers();

		static void GenerateBRDFLut();

		struct RendererData
		{
			Ref<CommandBuffer> commandBuffer;

			Ref<ShaderStorageBufferSet> indirectDrawBuffer;
			Ref<ShaderStorageBufferSet> indirectCountBuffer;
			Ref<ShaderStorageBufferSet> objectBuffer;

			Ref<RenderPipelineCompute> indirectCullPipeline;

			std::vector<RenderCommand> renderCommands;
			std::vector<IndirectBatch> indirectBatches;

			Ref<Camera> passCamera;
			Ref<RenderPass> currentPass;
			uint32_t passIndex = 0;

			std::vector<Ref<Material>> frameUpdatedMaterials;
			
			Skybox skyboxData;
			BindlessData bindlessData;

			std::vector<VkDescriptorPool> descriptorPools;

			/////Uniform data//////
			DirectionalLightData directionalLight;
			///////////////////////
		};
		
		inline static Scope<DefaultData> s_defaultData;
		inline static Scope<RendererData> s_rendererData;
		inline static std::vector<FunctionQueue> s_frameDeletionQueues;
		inline static std::vector<FunctionQueue> s_invalidationQueues;
	};
}