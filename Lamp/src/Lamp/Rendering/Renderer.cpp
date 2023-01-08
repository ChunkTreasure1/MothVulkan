
#include "lppch.h"
#include "Renderer.h"

#include "Lamp/Asset/Mesh/Material.h"
#include "Lamp/Asset/Mesh/Mesh.h"
#include "Lamp/Asset/AssetManager.h"

#include "Lamp/Core/Application.h"
#include "Lamp/Core/Window.h"
#include "Lamp/Core/Graphics/Swapchain.h"
#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"

#include "Lamp/Log/Log.h"

#include "Lamp/Rendering/Buffer/CommandBuffer.h"
#include "Lamp/Rendering/RenderPass/RenderPass.h"
#include "Lamp/Rendering/Framebuffer.h"

#include "Lamp/Rendering/Buffer/ShaderStorageBuffer/ShaderStorageBufferRegistry.h"
#include "Lamp/Rendering/Buffer/ShaderStorageBuffer/ShaderStorageBufferSet.h"

#include "Lamp/Rendering/Buffer/UniformBuffer/UniformBufferRegistry.h"
#include "Lamp/Rendering/Buffer/UniformBuffer/UniformBufferSet.h"
#include "Lamp/Rendering/Buffer/UniformBuffer/UniformBuffer.h"

#include "Lamp/Rendering/Buffer/IndexBuffer.h"
#include "Lamp/Rendering/Buffer/VertexBuffer.h"

#include "Lamp/Rendering/Buffer/CombinedIndexBuffer.h"
#include "Lamp/Rendering/Buffer/CombinedVertexBuffer.h"
#include "Lamp/Rendering/Texture/TextureTable.h"
#include "Lamp/Rendering/MaterialTable.h"

#include "Lamp/Rendering/Camera/Camera.h"

#include "Lamp/Rendering/Texture/SamplerLibrary.h"
#include "Lamp/Rendering/Texture/Texture2D.h"

#include "Lamp/Rendering/Shader/ShaderRegistry.h"

#include "Lamp/Rendering/RenderPipeline/RenderPipelineCompute.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipeline.h"

#include "Lamp/Rendering/RenderPass/RenderPassRegistry.h"
#include "Lamp/Rendering/RenderPass/RenderPass.h"

#include "Lamp/Rendering/DependencyGraph.h"

#include "Lamp/Utility/Math.h"
#include "Lamp/Utility/ImageUtility.h"

#define DEFAULT_IRRADIANCE_SET 0
#define DEFAULT_IRRADIANCE_BINDING 2

#define DEFAULT_RADIANCE_SET 0
#define DEFAULT_RADIANCE_BINDING 3

#define DEFAULT_BRDF_SET 0
#define DEFAULT_BRDF_BINDING 4

namespace Lamp
{
	void Renderer::Initialize()
	{
		const uint32_t framesInFlight = Application::Get().GetWindow()->GetSwapchain().GetFramesInFlight();
		s_rendererData->commandBuffer = CommandBuffer::Create(framesInFlight, false);

		CreateBindlessData();
		CreateDefaultData();
		CreateDescriptorPools();

		s_rendererData->skyboxData.irradianceMap = s_defaultData->blackCubeImage;
		s_rendererData->skyboxData.radianceMap = s_defaultData->blackCubeImage;
	}

	void Renderer::InitializeBuffers()
	{
		const uint32_t framesInFlight = Application::Get().GetWindow()->GetSwapchain().GetFramesInFlight();
		constexpr uint32_t MAX_OBJECT_COUNT = 20000;
		constexpr uint32_t PASS_COUNT = 3;

		s_rendererData = CreateScope<RendererData>();
		s_frameDeletionQueues.resize(framesInFlight);
		s_invalidationQueues.resize(framesInFlight);

		UniformBufferRegistry::Register(1, 0, UniformBufferSet::Create(sizeof(CameraData), PASS_COUNT, framesInFlight));
		UniformBufferRegistry::Register(1, 1, UniformBufferSet::Create(sizeof(TargetData), PASS_COUNT, framesInFlight));
		UniformBufferRegistry::Register(1, 2, UniformBufferSet::Create(sizeof(PassData), PASS_COUNT, framesInFlight));
		UniformBufferRegistry::Register(1, 3, UniformBufferSet::Create(sizeof(DirectionalLightData), framesInFlight));

		ShaderStorageBufferRegistry::Register(0, 1, ShaderStorageBufferSet::Create(sizeof(MaterialData) * 128, framesInFlight));

		s_defaultData = CreateScope<DefaultData>();

		CreateSamplers();

		// Textures
		{
			uint32_t blackCubeTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };

			ImageSpecification imageSpec{};
			imageSpec.format = ImageFormat::RGBA;
			imageSpec.width = 1;
			imageSpec.height = 1;
			imageSpec.usage = ImageUsage::Texture;
			imageSpec.layers = 6;
			imageSpec.isCubeMap = true;

			s_defaultData->blackCubeImage = Image2D::Create(imageSpec, blackCubeTextureData);

			uint32_t whiteTextureData = 0xffffffff;
			s_defaultData->whiteTexture = Texture2D::Create(ImageFormat::RGBA, 1, 1, &whiteTextureData);
			s_defaultData->whiteTexture->handle = Asset::Null();
		}
	}

	void Renderer::Shutdowm()
	{
		for (auto& descriptorPool : s_rendererData->descriptorPools)
		{
			vkDestroyDescriptorPool(GraphicsContext::GetDevice()->GetHandle(), descriptorPool, nullptr);
		}

		s_defaultData = nullptr;
		s_rendererData = nullptr;

		for (auto& s_frameDeletionQueue : s_frameDeletionQueues)
		{
			s_frameDeletionQueue.Flush();
		}

		SamplerLibrary::Shutdown();
	}

	void Renderer::Begin()
	{
		LP_PROFILE_FUNCTION();

		const uint32_t currentFrame = Application::Get().GetWindow()->GetSwapchain().GetCurrentFrame();
		LP_VK_CHECK(vkResetDescriptorPool(GraphicsContext::GetDevice()->GetHandle(), s_rendererData->descriptorPools[currentFrame], 0));

		s_rendererData->commandBuffer->Begin();

		UpdatePerFrameBuffers();

		{
			auto& data = GetBindlessData();
			data.combinedVertexBuffer->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer());
			data.combinedIndexBuffer->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer());
		}

		LP_PROFILE_GPU_EVENT("Rendering Begin");

		s_frameDeletionQueues[currentFrame].Flush();
		s_invalidationQueues[currentFrame].Flush();
	}

	void Renderer::End()
	{
		LP_PROFILE_FUNCTION();
		LP_PROFILE_GPU_EVENT("Rendering Begin");
		s_rendererData->commandBuffer->End();
		s_rendererData->renderCommands.clear();
	}

	void Renderer::ExecuteComputePass()
	{
		const Ref<RenderPass> currentPass = s_rendererData->currentPass;
		const uint32_t currentFrame = Application::Get().GetWindow()->GetSwapchain().GetCurrentFrame();
		const VkCommandBuffer currentCommandBuffer = s_rendererData->commandBuffer->GetCurrentCommandBuffer();

		currentPass->computePipeline->Bind(currentCommandBuffer, currentFrame);

		for (const auto& input : currentPass->computePipeline->GetFramebufferInputs())
		{
			currentPass->computePipeline->SetImage(input.framebuffer->GetColorAttachment(input.attachmentIndex), input.set, input.binding);
		}

		currentPass->computePipeline->SetImage(s_rendererData->skyboxData.irradianceMap, 2, 3);
		currentPass->computePipeline->SetImage(s_rendererData->skyboxData.radianceMap, 2, 4);
		currentPass->computePipeline->SetImage(s_defaultData->brdfLut, 2, 5);

		const uint32_t width = currentPass->framebuffer->GetWidth();
		const uint32_t height = currentPass->framebuffer->GetHeight();

		const uint32_t threadCountXY = 32;

		const uint32_t groupX = width / threadCountXY + 1;
		const uint32_t groupY = height / threadCountXY + 1;

		currentPass->framebuffer->Clear(currentCommandBuffer);

		currentPass->computePipeline->SetImage(currentPass->framebuffer->GetColorAttachment(0), 2, 6, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL);
		currentPass->computePipeline->Dispatch(currentCommandBuffer, currentFrame, groupX, groupY, 1, s_rendererData->passIndex);

		currentPass->computePipeline->InsertExecutionBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, currentFrame);
	}

	void Renderer::BeginPass(Ref<RenderPass> renderPass, Ref<Camera> camera)
	{
		LP_PROFILE_FUNCTION();
		LP_PROFILE_GPU_EVENT(std::string("Begin " + renderPass->name).c_str());
		s_rendererData->passCamera = camera;
		s_rendererData->currentPass = renderPass;

		UpdatePerPassBuffers();

		// Begin RenderPass
		if (!renderPass->computePipeline)
		{
			auto framebuffer = renderPass->framebuffer;
			framebuffer->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer());

			VkRenderingInfo renderingInfo{};
			renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
			renderingInfo.renderArea = { 0, 0, framebuffer->GetWidth(), framebuffer->GetHeight() };
			renderingInfo.layerCount = 1;
			renderingInfo.colorAttachmentCount = (uint32_t)framebuffer->GetColorAttachmentInfos().size();
			renderingInfo.pColorAttachments = framebuffer->GetColorAttachmentInfos().data();

			if (framebuffer->GetDepthAttachment())
			{
				renderingInfo.pDepthAttachment = &framebuffer->GetDepthAttachmentInfo();
			}
			else
			{
				renderingInfo.pDepthAttachment = nullptr;
			}
			renderingInfo.pStencilAttachment = nullptr;
			vkCmdBeginRendering(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), &renderingInfo);
		}
	}

	void Renderer::EndPass()
	{
		LP_PROFILE_FUNCTION();
		LP_PROFILE_GPU_EVENT(std::string("End " + s_rendererData->currentPass->name).c_str());

		if (!s_rendererData->currentPass->computePipeline)
		{
			vkCmdEndRendering(s_rendererData->commandBuffer->GetCurrentCommandBuffer());
		}

		s_rendererData->currentPass->framebuffer->Unbind(s_rendererData->commandBuffer->GetCurrentCommandBuffer());

		s_rendererData->currentPass = nullptr;
		s_rendererData->passCamera = nullptr;
		s_rendererData->passIndex++;
	}

	void Renderer::Submit(Ref<Mesh> mesh, const glm::mat4& transform)
	{
		LP_PROFILE_FUNCTION();
		for (const auto& subMesh : mesh->GetSubMeshes())
		{
			auto& cmd = s_rendererData->renderCommands.emplace_back();
			cmd.mesh = mesh;
			cmd.material = mesh->GetMaterial()->GetMaterials().at(subMesh.materialIndex);
			cmd.subMesh = subMesh;
			cmd.transform = transform;
		}
	}

	void Renderer::SubmitDirectionalLight(const glm::mat4& transform, const glm::vec3& color, const float intensity)
	{
		const glm::vec3 direction = glm::normalize(glm::mat3(transform) * glm::vec3(1.f)) * -1.f;
		s_rendererData->directionalLight.direction = { direction.x, direction.y, direction.z, 0.f };
		s_rendererData->directionalLight.colorIntensity = { color, intensity };
	}

	void Renderer::SubmitEnvironment(const Skybox& environment)
	{
		s_rendererData->skyboxData = environment;
		if (!s_rendererData->skyboxData.radianceMap)
		{
			s_rendererData->skyboxData.radianceMap = s_defaultData->blackCubeImage;
		}

		if (!s_rendererData->skyboxData.irradianceMap)
		{
			s_rendererData->skyboxData.irradianceMap = s_defaultData->blackCubeImage;
		}
	}

	void Renderer::DispatchRenderCommands()
	{
		LP_PROFILE_FUNCTION();
		if (s_rendererData->renderCommands.empty())
		{
			return;
		}

		const uint32_t currentFrame = s_rendererData->commandBuffer->GetCurrentIndex();

		// Draw
		{
			std::vector<IndirectBatch>& draws = s_rendererData->indirectBatches;
			for (uint32_t i = 0; i < draws.size(); i++)
			{
				LP_PROFILE_GPU_EVENT("DrawIndirect");

				{
					LP_PROFILE_SCOPE("Pipeline check");

					if (!s_rendererData->currentPass->excludedPipelineHashes.empty())
					{
						auto it = std::find(s_rendererData->currentPass->excludedPipelineHashes.begin(), s_rendererData->currentPass->excludedPipelineHashes.end(), draws[i].material->GetPipelineHash());
						if (it != s_rendererData->currentPass->excludedPipelineHashes.end())
						{
							continue;
						}
					}

					if (s_rendererData->currentPass->exclusivePipelineHash != 0 && draws[i].material->GetPipelineHash() != s_rendererData->currentPass->exclusivePipelineHash)
					{
						continue;
					}
				}

				if (i == 0 || (i > 0 && draws[i].material != draws[i - 1].material))
				{
					draws[i].material->UpdateInternalTexture(DEFAULT_IRRADIANCE_SET, DEFAULT_IRRADIANCE_BINDING, currentFrame, s_rendererData->skyboxData.irradianceMap);
					draws[i].material->UpdateInternalTexture(DEFAULT_RADIANCE_SET, DEFAULT_RADIANCE_BINDING, currentFrame, s_rendererData->skyboxData.radianceMap);
					draws[i].material->UpdateInternalTexture(DEFAULT_BRDF_SET, DEFAULT_BRDF_BINDING, currentFrame, s_defaultData->brdfLut);

					draws[i].material->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), currentFrame, s_rendererData->passIndex);
				}

				if (i == 0 || (i > 0 && draws[i].mesh != draws[i - 1].mesh))
				{
					//draws[i].mesh->GetVertexBuffer()->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer());
					//draws[i].mesh->GetIndexBuffer()->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer());
				}

				const VkDeviceSize drawOffset = draws[i].first * sizeof(GPUIndirectObject);
				const VkDeviceSize countOffset = i * sizeof(uint32_t);
				const uint32_t drawStride = sizeof(GPUIndirectObject);

				const Ref<ShaderStorageBuffer> currentIndirectBuffer = s_rendererData->indirectDrawBuffer->Get(currentFrame);
				const Ref<ShaderStorageBuffer> currentCountBuffer = s_rendererData->indirectCountBuffer->Get(currentFrame);

				vkCmdDrawIndexedIndirectCount(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), currentIndirectBuffer->GetHandle(), drawOffset, currentCountBuffer->GetHandle(), countOffset, draws[i].count, drawStride);
			}
		}
	}

	void Renderer::DispatchRenderCommandsTest()
	{
		const auto currentCommandBuffer = s_rendererData->commandBuffer->GetCurrentCommandBuffer();
		const uint32_t currentFrame = s_rendererData->commandBuffer->GetCurrentIndex();

		for (const auto& cmd : s_rendererData->renderCommands)
		{
			cmd.material->Bind(currentCommandBuffer, currentFrame);

			struct PushConstants
			{
				glm::mat4 transform;
				uint32_t materialId;

			} pushConsts;

			pushConsts.transform = cmd.transform;
			pushConsts.materialId = GetBindlessData().materialTable->GetMaterialId(cmd.material);

			cmd.material->SetPushConstant(currentCommandBuffer, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pushConsts);
			vkCmdDrawIndexed(currentCommandBuffer, cmd.subMesh.indexCount, 1, cmd.subMesh.indexStartOffset, cmd.subMesh.vertexStartOffset, 0);
		}
	}

	void Renderer::SubmitResourceFree(std::function<void()>&& function)
	{
		const uint32_t currentFrame = Application::Get().GetWindow()->GetSwapchain().GetCurrentFrame();
		s_frameDeletionQueues[currentFrame].Push(function);
	}

	void Renderer::SubmitInvalidation(std::function<void()>&& function)
	{
		const uint32_t currentFrame = Application::Get().GetWindow()->GetSwapchain().GetCurrentFrame();
		s_invalidationQueues[currentFrame].Push(function);
	}

	Skybox Renderer::GenerateEnvironmentMap(AssetHandle handle)
	{
		Ref<Texture2D> equirectangularTexture = AssetManager::GetAsset<Texture2D>(handle);
		if (!equirectangularTexture || !equirectangularTexture->IsValid())
		{
			return Skybox{};
		}

		constexpr uint32_t cubeMapSize = 1024;
		constexpr uint32_t irradianceMapSize = 32;
		constexpr uint32_t conversionThreadCount = 32;

		auto device = GraphicsContext::GetDevice();

		Ref<Image2D> environmentUnfiltered;
		Ref<Image2D> environmentFiltered;
		Ref<Image2D> irradianceMap;

		// Unfiltered - Conversion
		{
			ImageSpecification imageSpec{};
			imageSpec.format = ImageFormat::RGBA32F;
			imageSpec.width = cubeMapSize;
			imageSpec.height = cubeMapSize;
			imageSpec.usage = ImageUsage::Storage;
			imageSpec.layers = 6;
			imageSpec.isCubeMap = true;

			environmentUnfiltered = Image2D::Create(imageSpec);

			Ref<RenderPipelineCompute> conversionPipeline = RenderPipelineCompute::Create(ShaderRegistry::Get("EquirectangularConversion"));

			VkCommandBuffer cmdBuffer = device->GetCommandBuffer(true);
			environmentUnfiltered->TransitionToLayout(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);

			conversionPipeline->Bind(cmdBuffer);
			conversionPipeline->SetTexture(equirectangularTexture, 0, 0);
			conversionPipeline->SetImage(environmentUnfiltered, 0, 1, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			conversionPipeline->Dispatch(cmdBuffer, 0, cubeMapSize / conversionThreadCount, cubeMapSize / conversionThreadCount, 6);
			conversionPipeline->InsertBarrier(cmdBuffer, 0, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

			device->FlushCommandBuffer(cmdBuffer);
		}

		// Filtered
		{
			ImageSpecification imageSpec{};
			imageSpec.format = ImageFormat::RGBA32F;
			imageSpec.width = cubeMapSize;
			imageSpec.height = cubeMapSize;
			imageSpec.usage = ImageUsage::Storage;
			imageSpec.layers = 6;
			imageSpec.isCubeMap = true;
			imageSpec.mips = Utility::CalculateMipCount(cubeMapSize, cubeMapSize);

			environmentFiltered = Image2D::Create(imageSpec);

			for (uint32_t i = 0; i < environmentFiltered->GetMipCount(); i++)
			{
				environmentFiltered->CreateMipView(i);
			}

			{
				VkCommandBuffer cmdBuffer = device->GetCommandBuffer(true);
				environmentFiltered->TransitionToLayout(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);
				device->FlushCommandBuffer(cmdBuffer);
			}

			Ref<RenderPipelineCompute> filterPipeline = RenderPipelineCompute::Create(ShaderRegistry::Get("EnvironmentFiltering"));

			const float deltaRoughness = 1.f / glm::max((float)environmentFiltered->GetMipCount() - 1.f, 1.f);
			for (uint32_t i = 0, size = cubeMapSize; i < environmentFiltered->GetMipCount(); i++, size /= 2)
			{
				const uint32_t numGroups = glm::max(1u, size / 32);

				float roughness = i * deltaRoughness;
				roughness = glm::max(roughness, 0.05f);

				VkCommandBuffer cmdBuffer = device->GetCommandBuffer(true);

				filterPipeline->Bind(cmdBuffer);

				filterPipeline->SetImage(environmentUnfiltered, 0, 0, 0);
				filterPipeline->SetPushConstant(cmdBuffer, sizeof(float), &roughness);
				filterPipeline->SetImage(environmentFiltered, 0, 1, i, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL);

				filterPipeline->Dispatch(cmdBuffer, 0, numGroups, numGroups, 6);
				filterPipeline->InsertBarrier(cmdBuffer, 0, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

				device->FlushCommandBuffer(cmdBuffer);
			}
		}

		// Irradiance
		{
			ImageSpecification imageSpec{};
			imageSpec.format = ImageFormat::RGBA32F;
			imageSpec.width = irradianceMapSize;
			imageSpec.height = irradianceMapSize;
			imageSpec.usage = ImageUsage::Storage;
			imageSpec.layers = 6;
			imageSpec.isCubeMap = true;
			imageSpec.copyable = true;
			imageSpec.mips = Utility::CalculateMipCount(irradianceMapSize, irradianceMapSize);

			irradianceMap = Image2D::Create(imageSpec);

			Ref<RenderPipelineCompute> irradiancePipeline = RenderPipelineCompute::Create(ShaderRegistry::Get("EnvironmentGenerateIrradiance"));
			constexpr uint32_t irradianceComputeSamples = 512;

			VkCommandBuffer cmdBuffer = device->GetCommandBuffer(true);
			environmentFiltered->TransitionToLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			irradianceMap->TransitionToLayout(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);

			irradiancePipeline->Bind(cmdBuffer);
			irradiancePipeline->SetImage(environmentFiltered, 0, 0, 0);
			irradiancePipeline->SetImage(irradianceMap, 0, 1, 0, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			irradiancePipeline->SetPushConstant(cmdBuffer, sizeof(uint32_t), &irradianceComputeSamples);
			irradiancePipeline->Dispatch(cmdBuffer, 0, irradianceMapSize / conversionThreadCount, irradianceMapSize / conversionThreadCount, 6);
			irradiancePipeline->InsertBarrier(cmdBuffer, 0, VK_PIPELINE_STAGE_TRANSFER_BIT);

			irradianceMap->GenerateMips(false, cmdBuffer);
			irradianceMap->TransitionToLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			device->FlushCommandBuffer(cmdBuffer);
		}

		Skybox skybox{};
		skybox.irradianceMap = irradianceMap;
		skybox.radianceMap = environmentFiltered;

		return skybox;
	}

	VkDescriptorSet Renderer::AllocateDescriptorSet(VkDescriptorSetAllocateInfo& allocInfo)
	{
		LP_PROFILE_FUNCTION();

		auto device = GraphicsContext::GetDevice();
		uint32_t currentFrame = Application::Get().GetWindow()->GetSwapchain().GetCurrentFrame();

		allocInfo.descriptorPool = s_rendererData->descriptorPools[currentFrame];

		VkDescriptorSet descriptorSet;
		LP_VK_CHECK(vkAllocateDescriptorSets(device->GetHandle(), &allocInfo, &descriptorSet));

		return descriptorSet;
	}

	void Renderer::CreateDefaultData()
	{
		// Default textures
		{
			GenerateBRDFLut();
		}

		// Default pipeline
		{
			RenderPipelineSpecification renderPipelineSpec{};
			renderPipelineSpec.name = "Default";
			renderPipelineSpec.shader = ShaderRegistry::Get("Default");
			renderPipelineSpec.framebuffer = RenderPassRegistry::Get("forward")->framebuffer;
			renderPipelineSpec.renderPass = "forward";
			renderPipelineSpec.vertexLayout =
			{
				{ ElementType::Float3, "a_position" },
				{ ElementType::Float3, "a_normal" },
				{ ElementType::Float3, "a_tangent" },
				{ ElementType::Float3, "a_bitangent" },
				{ ElementType::Float2, "a_texCoords" }
			};

			s_defaultData->defaultPipeline = RenderPipeline::Create(renderPipelineSpec);
		}

		s_defaultData->defaultMaterial = AssetManager::GetAsset<MultiMaterial>("Engine/Materials/default.lpmat");
	}

	void Renderer::CreateSamplers()
	{
		SamplerLibrary::Add(TextureFilter::Linear, TextureFilter::Linear, TextureFilter::Linear, TextureWrap::Repeat, CompareOperator::None, AniostopyLevel::None);
		SamplerLibrary::Add(TextureFilter::Nearest, TextureFilter::Linear, TextureFilter::Linear, TextureWrap::Repeat, CompareOperator::None, AniostopyLevel::None);
		SamplerLibrary::Add(TextureFilter::Nearest, TextureFilter::Nearest, TextureFilter::Linear, TextureWrap::Repeat, CompareOperator::None, AniostopyLevel::None);

		SamplerLibrary::Add(TextureFilter::Nearest, TextureFilter::Nearest, TextureFilter::Nearest, TextureWrap::Repeat, CompareOperator::None, AniostopyLevel::None);
		SamplerLibrary::Add(TextureFilter::Linear, TextureFilter::Nearest, TextureFilter::Nearest, TextureWrap::Repeat, CompareOperator::None, AniostopyLevel::None);
		SamplerLibrary::Add(TextureFilter::Linear, TextureFilter::Linear, TextureFilter::Nearest, TextureWrap::Repeat, CompareOperator::None, AniostopyLevel::None);

		SamplerLibrary::Add(TextureFilter::Linear, TextureFilter::Linear, TextureFilter::Linear, TextureWrap::Clamp, CompareOperator::None, AniostopyLevel::None);
		SamplerLibrary::Add(TextureFilter::Nearest, TextureFilter::Linear, TextureFilter::Linear, TextureWrap::Clamp, CompareOperator::None, AniostopyLevel::None);
		SamplerLibrary::Add(TextureFilter::Nearest, TextureFilter::Nearest, TextureFilter::Linear, TextureWrap::Clamp, CompareOperator::None, AniostopyLevel::None);

		SamplerLibrary::Add(TextureFilter::Nearest, TextureFilter::Nearest, TextureFilter::Nearest, TextureWrap::Clamp, CompareOperator::None, AniostopyLevel::None);
		SamplerLibrary::Add(TextureFilter::Linear, TextureFilter::Nearest, TextureFilter::Nearest, TextureWrap::Clamp, CompareOperator::None, AniostopyLevel::None);
		SamplerLibrary::Add(TextureFilter::Linear, TextureFilter::Linear, TextureFilter::Nearest, TextureWrap::Clamp, CompareOperator::None, AniostopyLevel::None);
	}

	void Renderer::CreateDescriptorPools()
	{
		VkDescriptorPoolSize poolSizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 10000;
		poolInfo.poolSizeCount = (uint32_t)ARRAYSIZE(poolSizes);
		poolInfo.pPoolSizes = poolSizes;

		const uint32_t framesInFlight = Application::Get().GetWindow()->GetSwapchain().GetFramesInFlight();

		s_rendererData->descriptorPools.resize(framesInFlight);
		auto device = GraphicsContext::GetDevice();

		for (auto& descriptorPool : s_rendererData->descriptorPools)
		{
			LP_VK_CHECK(vkCreateDescriptorPool(device->GetHandle(), &poolInfo, nullptr, &descriptorPool));
		}
	}

	void Renderer::CreateBindlessData()
	{
		auto& data = GetBindlessData();

		data.combinedVertexBuffer = CombinedVertexBuffer::Create(sizeof(Vertex), BindlessData::MAX_VERTICES);
		data.combinedIndexBuffer = CombinedIndexBuffer::Create(BindlessData::MAX_INDICES);
		data.textureTable = TextureTable::Create(16384, 0);
		data.materialTable = MaterialTable::Create();
	}

	void Renderer::UpdatePerPassBuffers()
	{
		LP_PROFILE_FUNCTION();

		const uint32_t currentFrame = s_rendererData->commandBuffer->GetCurrentIndex();

		// Update camera data
		{
			auto currentCameraBuffer = UniformBufferRegistry::Get(1, 0)->Get(currentFrame);
			CameraData* cameraData = currentCameraBuffer->Map<CameraData>();

			cameraData->proj = s_rendererData->passCamera->GetProjection();
			cameraData->view = s_rendererData->passCamera->GetView();
			cameraData->position = glm::vec4(s_rendererData->passCamera->GetPosition(), 1.f);
			cameraData->viewProj = cameraData->proj * cameraData->view;

			currentCameraBuffer->Unmap();
		}

		// Update target data
		{
			auto currentTargetBuffer = UniformBufferRegistry::Get(1, 1)->Get(currentFrame);
			TargetData* targetData = currentTargetBuffer->Map<TargetData>();
			targetData->targetSize = { s_rendererData->currentPass->framebuffer->GetWidth(), s_rendererData->currentPass->framebuffer->GetHeight() };

			currentTargetBuffer->Unmap();
		}

		// Update pass data
		{
			auto currentPassBuffer = UniformBufferRegistry::Get(1, 2)->Get(currentFrame);
			PassData* passData = currentPassBuffer->Map<PassData>();
			passData->passIndex = s_rendererData->passIndex;

			currentPassBuffer->Unmap();
		}
	}

	void Renderer::UpdatePerFrameBuffers()
	{
		const uint32_t currentFrame = s_rendererData->commandBuffer->GetCurrentIndex();
		const auto& currentMaterialBuffer = ShaderStorageBufferRegistry::Get(0, 11)->Get(currentFrame);
		auto& bindlessData = GetBindlessData();

		const auto& materialTable = bindlessData.materialTable->GetTable();

		MaterialData* data = currentMaterialBuffer->Map<MaterialData>();

		for (const auto& [material, id] : materialTable)
		{
			MaterialData& d = data[id];
			d.albedo = bindlessData.textureTable->GetBindingFromTexture(material->GetTextures().at(0));
			d.materialNormal = bindlessData.textureTable->GetBindingFromTexture(material->GetTextures().at(1));
		}

		currentMaterialBuffer->Unmap();
	}

	void Renderer::GenerateBRDFLut()
	{
		constexpr uint32_t brdfSize = 512;

		ImageSpecification imageSpec{};
		imageSpec.format = ImageFormat::RG16F;
		imageSpec.width = brdfSize;
		imageSpec.height = brdfSize;
		imageSpec.usage = ImageUsage::Storage;
		imageSpec.filter = TextureFilter::Linear;

		s_defaultData->brdfLut = Image2D::Create(imageSpec);

		Ref<RenderPipelineCompute> brdfPipeline = RenderPipelineCompute::Create(ShaderRegistry::Get("BRDFGeneration"));

		auto device = GraphicsContext::GetDevice();
		VkCommandBuffer cmdBuffer = device->GetCommandBuffer(true);
		s_defaultData->brdfLut->TransitionToLayout(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);

		brdfPipeline->Bind(cmdBuffer);
		brdfPipeline->SetImage(s_defaultData->brdfLut, 0, 0, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		brdfPipeline->Dispatch(cmdBuffer, 0, brdfSize / 32, brdfSize / 32, 1);
		brdfPipeline->InsertBarrier(cmdBuffer, 0, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		device->FlushCommandBuffer(cmdBuffer);
	}
}