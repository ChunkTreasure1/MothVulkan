
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
#include "Lamp/Rendering/Camera/Camera.h"
#include "Lamp/Rendering/Texture/Texture2D.h"

#include "Lamp/Rendering/Shader/ShaderRegistry.h"

#include "Lamp/Rendering/RenderPipeline/RenderPipelineCompute.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipeline.h"

#include "Lamp/Rendering/RenderPass/RenderPassRegistry.h"
#include "Lamp/Rendering/RenderPass/RenderPass.h"

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

		CreateDefaultData();
		CreateDescriptorPools();

		s_rendererData->skyboxData.irradianceMap = s_defaultData->blackCubeImage;
		s_rendererData->skyboxData.radianceMap = s_defaultData->blackCubeImage;
	}

	void Renderer::InitializeBuffers()
	{
		const uint32_t framesInFlight = Application::Get().GetWindow()->GetSwapchain().GetFramesInFlight();
		constexpr uint32_t MAX_OBJECT_COUNT = 200000;

		s_rendererData = CreateScope<RendererData>();
		s_frameDeletionQueues.resize(framesInFlight);
		s_invalidationQueues.resize(framesInFlight);

		UniformBufferRegistry::Register(0, 1, UniformBufferSet::Create(sizeof(DirectionalLightData), framesInFlight));
		UniformBufferRegistry::Register(1, 0, UniformBufferSet::Create(sizeof(CameraData), 3, framesInFlight));
		UniformBufferRegistry::Register(1, 1, UniformBufferSet::Create(sizeof(TargetData), 3, framesInFlight));

		ShaderStorageBufferRegistry::Register(4, 0, ShaderStorageBufferSet::Create(sizeof(ObjectData) * MAX_OBJECT_COUNT, framesInFlight));
		ShaderStorageBufferRegistry::Register(4, 1, ShaderStorageBufferSet::Create(sizeof(uint32_t) * MAX_OBJECT_COUNT, framesInFlight));

		s_rendererData->indirectDrawBuffer = ShaderStorageBufferSet::Create(sizeof(GPUIndirectObject) * MAX_OBJECT_COUNT, framesInFlight, true);
		s_rendererData->indirectCountBuffer = ShaderStorageBufferSet::Create(sizeof(uint32_t) * MAX_OBJECT_COUNT, framesInFlight, true);
		s_rendererData->indirectCullPipeline = RenderPipelineCompute::Create(Shader::Create("ComputeCull", { "Engine/Shaders/cull_cs.glsl" }), framesInFlight);

		s_rendererData->indirectCullPipeline->SetStorageBuffer(s_rendererData->indirectDrawBuffer, 0, 1, VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
		s_rendererData->indirectCullPipeline->SetStorageBuffer(s_rendererData->indirectCountBuffer, 0, 2, VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
		s_rendererData->indirectCullPipeline->SetStorageBuffer(ShaderStorageBufferRegistry::Get(4, 1), 0, 3, VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
		s_rendererData->indirectCullPipeline->SetStorageBuffer(ShaderStorageBufferRegistry::Get(4, 0), 0, 4, VK_ACCESS_INDIRECT_COMMAND_READ_BIT);

		s_defaultData = CreateScope<DefaultData>();

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
	}

	void Renderer::Begin()
	{
		LP_PROFILE_FUNCTION();

		const uint32_t currentFrame = Application::Get().GetWindow()->GetSwapchain().GetCurrentFrame();
		LP_VK_CHECK(vkResetDescriptorPool(GraphicsContext::GetDevice()->GetHandle(), s_rendererData->descriptorPools[currentFrame], 0));

		s_rendererData->commandBuffer->Begin();
		s_rendererData->indirectCullPipeline->WriteAndBindDescriptors(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), currentFrame);
		s_rendererData->frameUpdatedMaterials.clear();
		s_rendererData->passIndex = 0;

		s_frameDeletionQueues[currentFrame].Flush();
		s_invalidationQueues[currentFrame].Flush();


		SortRenderCommands();
		PrepareForIndirectDraw(s_rendererData->renderCommands);
		UploadRenderCommands();
		UpdatePerFrameBuffers();
	}

	void Renderer::End()
	{
		LP_PROFILE_FUNCTION();
		s_rendererData->commandBuffer->End();
		s_rendererData->renderCommands.clear();
	}

	void Renderer::ExecuteComputePass()
	{
		UpdatePerPassBuffers();

		const Ref<RenderPass> currentPass = s_rendererData->currentPass;
		const uint32_t currentFrame = Application::Get().GetWindow()->GetSwapchain().GetCurrentFrame();

		currentPass->computePipeline->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), currentFrame);

		for (const auto& input : currentPass->computePipeline->GetFramebufferInputs())
		{
			currentPass->computePipeline->SetImage(input.framebuffer->GetColorAttachment(input.attachmentIndex), input.set, input.binding);
		}

		const uint32_t width = currentPass->framebuffer->GetWidth();
		const uint32_t height = currentPass->framebuffer->GetHeight();

		const uint32_t threadCountXY = 32;

		const uint32_t groupX = width / threadCountXY + 1;
		const uint32_t groupY = height / threadCountXY + 1;
	
		currentPass->computePipeline->SetImage(currentPass->framebuffer->GetColorAttachment(0), 2, 6, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL);
		currentPass->computePipeline->Dispatch(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), currentFrame, groupX, groupY, 1, s_rendererData->passIndex);

		currentPass->computePipeline->InsertExecutionBarrier(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, currentFrame);
	}

	void Renderer::BeginPass(Ref<RenderPass> renderPass, Ref<Camera> camera)
	{
		LP_PROFILE_FUNCTION();
		s_rendererData->passCamera = camera;
		s_rendererData->currentPass = renderPass;

		UpdatePerPassBuffers();
		// Begin RenderPass
		if (!renderPass->computePipeline)
		{
			CullRenderCommands();

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

		if (!s_rendererData->currentPass->computePipeline)
		{
			vkCmdEndRendering(s_rendererData->commandBuffer->GetCurrentCommandBuffer());
			s_rendererData->currentPass->framebuffer->Unbind(s_rendererData->commandBuffer->GetCurrentCommandBuffer());
		}

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

				draws[i].mesh->GetVertexBuffer()->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer());
				draws[i].mesh->GetIndexBuffer()->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer());

				const VkDeviceSize drawOffset = draws[i].first * sizeof(GPUIndirectObject);
				const VkDeviceSize countOffset = i * sizeof(uint32_t);
				const uint32_t drawStride = sizeof(GPUIndirectObject);

				const Ref<ShaderStorageBuffer> currentIndirectBuffer = s_rendererData->indirectDrawBuffer->Get(currentFrame);
				const Ref<ShaderStorageBuffer> currentCountBuffer = s_rendererData->indirectCountBuffer->Get(currentFrame);

				vkCmdDrawIndexedIndirectCount(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), currentIndirectBuffer->GetHandle(), drawOffset, currentCountBuffer->GetHandle(), countOffset, draws[i].count, drawStride);
			}
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

	void Renderer::PrepareForIndirectDraw(std::vector<RenderCommand>& renderCommands)
	{
		LP_PROFILE_FUNCTION();

		if (renderCommands.empty())
		{
			return;
		}

		s_rendererData->indirectBatches.clear();
		auto& draws = s_rendererData->indirectBatches;

		IndirectBatch& firstDraw = draws.emplace_back();
		firstDraw.mesh = renderCommands[0].mesh;
		firstDraw.material = renderCommands[0].material;
		firstDraw.subMesh = renderCommands[0].subMesh;
		firstDraw.first = 0;
		firstDraw.count = 1;

		for (uint32_t i = 1; i < renderCommands.size(); i++)
		{
			bool sameMesh = (renderCommands[i].mesh == draws.back().mesh);
			bool sameSubMesh = (renderCommands[i].subMesh == draws.back().subMesh);
			bool sameMaterial = (renderCommands[i].material == draws.back().material);

			if (sameMesh && sameMaterial && sameSubMesh)
			{
				draws.back().count++;
			}
			else
			{
				IndirectBatch& newDraw = draws.emplace_back();
				newDraw.mesh = renderCommands[i].mesh;
				newDraw.material = renderCommands[i].material;
				newDraw.subMesh = renderCommands[i].subMesh;
				newDraw.first = i;
				newDraw.count = 1;
				newDraw.id = uint32_t(draws.size() - 1);
			}
		}

		std::sort(draws.begin(), draws.end(), [](const IndirectBatch& lhs, const IndirectBatch& rhs)
			{
				return lhs.material < rhs.material;
			});
	}

	void Renderer::UpdatePerPassBuffers()
	{
		LP_PROFILE_FUNCTION();

		const uint32_t currentFrame = s_rendererData->commandBuffer->GetCurrentIndex();

		// Update camera data
		{
			auto currentCameraBuffer = UniformBufferRegistry::Get(1, 0)->Get(currentFrame);
			uint8_t* bytePtr = currentCameraBuffer->Map<uint8_t>();

			const uint32_t ptrOffset = currentCameraBuffer->GetSize() * s_rendererData->passIndex;
			bytePtr += ptrOffset;

			CameraData* cameraData = (CameraData*)bytePtr;

			cameraData->proj = s_rendererData->passCamera->GetProjection();
			cameraData->view = s_rendererData->passCamera->GetView();
			cameraData->position = glm::vec4(s_rendererData->passCamera->GetPosition(), 1.f);
			cameraData->viewProj = cameraData->proj * cameraData->view;

			currentCameraBuffer->Unmap();
		}

		// Update target data
		{
			auto currentTargetBuffer = UniformBufferRegistry::Get(1, 1)->Get(currentFrame);
			uint8_t* bytePtr = currentTargetBuffer->Map<uint8_t>();

			const uint32_t ptrOffset = currentTargetBuffer->GetSize() * s_rendererData->passIndex;
			bytePtr += ptrOffset;

			TargetData* targetData = (TargetData*)bytePtr;
			targetData->targetSize = { s_rendererData->currentPass->framebuffer->GetWidth(), s_rendererData->currentPass->framebuffer->GetHeight() };

			currentTargetBuffer->Unmap();
		}
	}

	void Renderer::UpdatePerFrameBuffers()
	{
		LP_PROFILE_FUNCTION();

		const uint32_t currentFrame = s_rendererData->commandBuffer->GetCurrentIndex();

		// Update object data
		{
			auto currentObjectBuffer = ShaderStorageBufferRegistry::Get(4, 0)->Get(currentFrame);
			auto* objectData = currentObjectBuffer->Map<ObjectData>();

			for (uint32_t i = 0; i < s_rendererData->renderCommands.size(); i++)
			{
				const BoundingSphere& boundingSphere = s_rendererData->renderCommands[i].mesh->GetBoundingSphere();

				const glm::mat4 transform = s_rendererData->renderCommands[i].transform;
				const glm::vec3 globalScale = { glm::length(transform[0]), glm::length(transform[1]), glm::length(transform[2]) };
				const glm::vec3 globalCenter = transform * glm::vec4(boundingSphere.center, 1.f);
				const float maxScale = std::max(globalScale.x, std::max(globalScale.y, globalScale.z));

				objectData[i].transform = transform;
				objectData[i].sphereBounds = glm::vec4(globalCenter, boundingSphere.radius * maxScale * 0.5f);
			}

			currentObjectBuffer->Unmap();
		}

		// Update directional light
		{
			auto currentBuffer = UniformBufferRegistry::Get(0, 1)->Get(currentFrame);
			currentBuffer->SetData(&s_rendererData->directionalLight, sizeof(DirectionalLightData));
		}
	}

	void Renderer::SortRenderCommands()
	{
		LP_PROFILE_FUNCTION();
		std::sort(s_rendererData->renderCommands.begin(), s_rendererData->renderCommands.end(), [](const RenderCommand& lhs, const RenderCommand& rhs)
			{
				return lhs.subMesh > rhs.subMesh;
			});
	}

	void Renderer::UploadRenderCommands()
	{
		LP_PROFILE_FUNCTION();

		const uint32_t currentFrame = s_rendererData->commandBuffer->GetCurrentIndex();
		std::vector<IndirectBatch>& draws = s_rendererData->indirectBatches;

		// Fill indirect commands
		{
			auto* drawCommands = s_rendererData->indirectDrawBuffer->Get(currentFrame)->Map<GPUIndirectObject>();

			for (uint32_t i = 0; i < s_rendererData->renderCommands.size(); i++)
			{
				auto& cmd = s_rendererData->renderCommands[i];

				drawCommands[i].command.indexCount = cmd.subMesh.indexCount;
				drawCommands[i].command.firstIndex = cmd.subMesh.indexStartOffset;
				drawCommands[i].command.vertexOffset = cmd.subMesh.vertexStartOffset;
				drawCommands[i].command.instanceCount = 1;
				drawCommands[i].objectId = i;

				// TODO: this is probably not performant
				auto it = std::find_if(draws.begin(), draws.end(), [&cmd](const IndirectBatch& batch)
					{
						return batch.mesh == cmd.mesh && batch.material == cmd.material && batch.subMesh == cmd.subMesh;
					});

				if (it != draws.end())
				{
					drawCommands[i].batchId = it->id;
					drawCommands[i].command.firstInstance = it->first;
				}

			}

			s_rendererData->indirectDrawBuffer->Get(currentFrame)->Unmap();
		}

		{
			auto* ids = ShaderStorageBufferRegistry::Get(4, 1)->Get(currentFrame)->Map<uint32_t>();

			for (uint32_t i = 0; i < s_rendererData->renderCommands.size(); i++)
			{
				ids[i] = 0;
			}

			ShaderStorageBufferRegistry::Get(4, 1)->Get(currentFrame)->Unmap();
		}
	}

	void Renderer::CullRenderCommands()
	{
		const uint32_t currentFrame = s_rendererData->commandBuffer->GetCurrentIndex();

		auto* drawCount = s_rendererData->indirectCountBuffer->Get(currentFrame)->Map<uint32_t>();

		std::vector<IndirectBatch>& draws = s_rendererData->indirectBatches;

		for (uint32_t i = 0; i < draws.size(); i++)
		{
			drawCount[i] = 0;
		}

		s_rendererData->indirectCountBuffer->Get(currentFrame)->Unmap();
		s_rendererData->indirectCullPipeline->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), currentFrame);

		// Set cull data
		{
			glm::mat4 projection = s_rendererData->passCamera->GetProjection();
			glm::mat4 projectionTrans = glm::transpose(projection);

			glm::vec4 frustumX = Math::NormalizePlane(projectionTrans[3] + projectionTrans[0]);
			glm::vec4 frustumY = Math::NormalizePlane(projectionTrans[3] + projectionTrans[1]);

			CullData cullData{};
			cullData.view = s_rendererData->passCamera->GetView();
			cullData.P00 = projection[0][0];
			cullData.P11 = projection[1][1];
			cullData.zNear = s_rendererData->passCamera->GetNearPlane();
			cullData.zFar = s_rendererData->passCamera->GetFarPlane();

			cullData.frustum[0] = frustumX.x;
			cullData.frustum[1] = frustumX.z;
			cullData.frustum[2] = frustumY.y;
			cullData.frustum[3] = frustumY.z;

			cullData.drawCount = (uint32_t)s_rendererData->renderCommands.size();

			cullData.cullingEnabled = true;
			cullData.lodEnabled = true;
			cullData.distCull = 0;

			s_rendererData->indirectCullPipeline->SetPushConstant(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), sizeof(CullData), &cullData);
		}

		const uint32_t dispatchCount = (uint32_t)(s_rendererData->renderCommands.size() / 256) + 1;
		s_rendererData->indirectCullPipeline->DispatchNoUpdate(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), currentFrame, dispatchCount, 1, 1);
		s_rendererData->indirectCullPipeline->InsertBarrier(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), currentFrame, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);
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

		brdfPipeline->Bind(cmdBuffer);
		brdfPipeline->SetImage(s_defaultData->brdfLut, 0, 0, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		brdfPipeline->Dispatch(cmdBuffer, 0, brdfSize / 32, brdfSize / 32, 1);
		brdfPipeline->InsertBarrier(cmdBuffer, 0, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		device->FlushCommandBuffer(cmdBuffer);
	}
}