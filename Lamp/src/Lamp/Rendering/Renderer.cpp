
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

#include "Lamp/Utility/Math.h"

namespace Lamp
{
	void Renderer::Initialize()
	{
		s_defaultData = CreateScope<DefaultData>();

		const uint32_t framesInFlight = Application::Get().GetWindow()->GetSwapchain().GetFramesInFlight();
		s_rendererData->commandBuffer = CommandBuffer::Create(framesInFlight, false);

		s_rendererData->indirectCullPipeline = RenderPipelineCompute::Create(Shader::Create("ComputeCull", { "Engine/Shaders/cull_cs.glsl" }), framesInFlight);

		CreateDefaultData();
		CreateDescriptorPools();
	}

	void Renderer::InitializeBuffers()
	{
		const uint32_t framesInFlight = Application::Get().GetWindow()->GetSwapchain().GetFramesInFlight();
		constexpr uint32_t MAX_OBJECT_COUNT = 200000;

		s_rendererData = CreateScope<RendererData>();
		s_frameDeletionQueues.resize(framesInFlight);

		UniformBufferRegistry::Register(0, 0, UniformBufferSet::Create(sizeof(CameraData), framesInFlight));
		UniformBufferRegistry::Register(0, 1, UniformBufferSet::Create(sizeof(DirectionalLightData), framesInFlight));

		ShaderStorageBufferRegistry::Register(1, 0, ShaderStorageBufferSet::Create(sizeof(ObjectData) * MAX_OBJECT_COUNT, framesInFlight));
		ShaderStorageBufferRegistry::Register(1, 1, ShaderStorageBufferSet::Create(sizeof(uint32_t) * MAX_OBJECT_COUNT, framesInFlight));

		s_rendererData->indirectDrawBuffer = ShaderStorageBufferSet::Create(sizeof(GPUIndirectObject) * MAX_OBJECT_COUNT, framesInFlight, true);
		s_rendererData->indirectCountBuffer = ShaderStorageBufferSet::Create(sizeof(uint32_t) * MAX_OBJECT_COUNT, framesInFlight, true);
	}

	void Renderer::Shutdowm()
	{
		for (uint32_t i = 0; i < s_rendererData->descriptorPools.size(); i++)
		{
			vkDestroyDescriptorPool(GraphicsContext::GetDevice()->GetHandle(), s_rendererData->descriptorPools[i], nullptr);
		}

		s_defaultData = nullptr;
		s_rendererData = nullptr;

		for (size_t i = 0; i < s_frameDeletionQueues.size(); i++)
		{
			s_frameDeletionQueues[i].Flush();
		}
	}

	void Renderer::Begin()
	{
		LP_PROFILE_FUNCTION();

		GenerateEnvironmentMap("Assets/Textures/HDRIs/brightForest.hdr");

		uint32_t currentFrame = Application::Get().GetWindow()->GetSwapchain().GetCurrentFrame();
		LP_VK_CHECK(vkResetDescriptorPool(GraphicsContext::GetDevice()->GetHandle(), s_rendererData->descriptorPools[currentFrame], 0));

		s_rendererData->commandBuffer->Begin();
		s_frameDeletionQueues[currentFrame].Flush();

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

	void Renderer::BeginPass(Ref<RenderPass> renderPass, Ref<Camera> camera)
	{
		LP_PROFILE_FUNCTION();
		s_rendererData->passCamera = camera;

		CullRenderCommands();
		UpdatePerPassBuffers();

		// Begin RenderPass
		{
			auto framebuffer = renderPass->framebuffer;
			s_rendererData->currentFramebuffer = framebuffer;

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
		vkCmdEndRendering(s_rendererData->commandBuffer->GetCurrentCommandBuffer());
		s_rendererData->currentFramebuffer->Unbind(s_rendererData->commandBuffer->GetCurrentCommandBuffer());
		s_rendererData->currentFramebuffer = nullptr;
		s_rendererData->passCamera = nullptr;
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

			draws.front().material->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), currentFrame);
			for (uint32_t i = 0; i < draws.size(); i++)
			{
				if (i > 0 && draws[i].material != draws[i - 1].material)
				{
					draws[i].material->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), currentFrame);
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

	void Renderer::SubmitDestroy(std::function<void()>&& function)
	{
		const uint32_t currentFrame = Application::Get().GetWindow()->GetSwapchain().GetCurrentFrame();
		s_frameDeletionQueues[currentFrame].Push(function);
	}

	Skybox Renderer::GenerateEnvironmentMap(const std::filesystem::path& cubeMap)
	{
		Ref<Texture2D> equirectangularTexture = AssetManager::GetAsset<Texture2D>(cubeMap);
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

		VkCommandBuffer cmdBuffer = device->GetCommandBuffer(true);

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
			conversionPipeline->Bind(cmdBuffer);
			conversionPipeline->SetTexture(equirectangularTexture, 0, 0);
			conversionPipeline->SetImage(environmentUnfiltered, 0, 1, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			conversionPipeline->Dispatch(cmdBuffer, 0, cubeMapSize / conversionThreadCount, cubeMapSize / conversionThreadCount, 1);
			conversionPipeline->InsertBarrier(cmdBuffer, 0, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
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

			environmentFiltered = Image2D::Create(imageSpec);

			for (uint32_t i = 0; i < environmentFiltered->GetMipCount(); i++)
			{
				environmentFiltered->CreateMipView(i);
			}

			Ref<RenderPipelineCompute> filterPipeline = RenderPipelineCompute::Create(ShaderRegistry::Get("EnvironmentFiltering"));
			filterPipeline->Bind(cmdBuffer);
			filterPipeline->SetImage(environmentUnfiltered, 0, 0, 0, VK_ACCESS_SHADER_READ_BIT);
			
			const float deltaRoughness = 1.f / glm::max((float)environmentFiltered->GetMipCount() - 1.f, 1.f);
			for (uint32_t i = 0, size = cubeMapSize; i < environmentFiltered->GetMipCount(); i++, size /= 2)
			{
				const uint32_t numGroups = glm::max(1u, size / 32);

				float roughness = i * deltaRoughness;
				roughness = glm::max(roughness, 0.05f);

				filterPipeline->SetPushConstant(cmdBuffer, sizeof(float), &roughness);
				filterPipeline->SetImage(environmentFiltered, 0, 1, i, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL);
				filterPipeline->Dispatch(cmdBuffer, 0, numGroups, numGroups, 6);
			}

			filterPipeline->InsertBarrier(cmdBuffer, 0, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
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

			irradianceMap = Image2D::Create(imageSpec);
			
			Ref<RenderPipelineCompute> irradiancePipeline = RenderPipelineCompute::Create(ShaderRegistry::Get("EnvironmentGenerateIrradiance"));
			constexpr uint32_t irradianceComputeSamples = 512;

			irradiancePipeline->Bind(cmdBuffer);
			irradiancePipeline->SetImage(environmentFiltered, 0, 0, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			irradiancePipeline->SetImage(irradianceMap, 0, 1, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			irradiancePipeline->SetPushConstant(cmdBuffer, sizeof(uint32_t), &irradianceComputeSamples);
			irradiancePipeline->Dispatch(cmdBuffer, 0, irradianceMapSize / conversionThreadCount, irradianceMapSize / conversionThreadCount, 6);
			irradiancePipeline->InsertBarrier(cmdBuffer, 0, VK_PIPELINE_STAGE_TRANSFER_BIT);

			irradianceMap->GenerateMips(false);
		}

		device->FlushCommandBuffer(cmdBuffer);

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
			//uint32_t blackCubeTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
			//s_defaultData->blackCubeTexture = TextureCube::Create(ImageFormat::RGBA, 1, 1, &blackCubeTextureData);

			uint32_t whiteTextureData = 0xffffffff;
			s_defaultData->whiteTexture = Texture2D::Create(ImageFormat::RGBA, 1, 1, &whiteTextureData);
			s_defaultData->whiteTexture->handle = Asset::Null();
		}
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

		for (uint32_t i = 0; i < s_rendererData->descriptorPools.size(); ++i)
		{
			LP_VK_CHECK(vkCreateDescriptorPool(device->GetHandle(), &poolInfo, nullptr, &s_rendererData->descriptorPools[i]));
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
			auto currentCameraBuffer = UniformBufferRegistry::Get(0, 0)->Get(currentFrame);
			CameraData* cameraData = currentCameraBuffer->Map<CameraData>();

			cameraData->proj = s_rendererData->passCamera->GetProjection();
			cameraData->view = s_rendererData->passCamera->GetView();
			cameraData->viewProj = cameraData->proj * cameraData->view;

			currentCameraBuffer->Unmap();
		}
	}

	void Renderer::UpdatePerFrameBuffers()
	{
		LP_PROFILE_FUNCTION();

		const uint32_t currentFrame = s_rendererData->commandBuffer->GetCurrentIndex();

		// Update object data
		{
			auto currentObjectBuffer = ShaderStorageBufferRegistry::Get(1, 0)->Get(currentFrame);
			ObjectData* objectData = currentObjectBuffer->Map<ObjectData>();

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
			return lhs.subMesh > rhs.subMesh || lhs.mesh > rhs.mesh;
		});
	}

	void Renderer::UploadRenderCommands()
	{
		LP_PROFILE_FUNCTION();

		const uint32_t currentFrame = s_rendererData->commandBuffer->GetCurrentIndex();
		std::vector<IndirectBatch>& draws = s_rendererData->indirectBatches;

		// Fill indirect commands
		{
			GPUIndirectObject* drawCommands = s_rendererData->indirectDrawBuffer->Get(currentFrame)->Map<GPUIndirectObject>();

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
			uint32_t* ids = ShaderStorageBufferRegistry::Get(1, 1)->Get(currentFrame)->Map<uint32_t>();

			for (uint32_t i = 0; i < s_rendererData->renderCommands.size(); i++)
			{
				ids[i] = 0;
			}

			ShaderStorageBufferRegistry::Get(1, 1)->Get(currentFrame)->Unmap();
		}
	}

	void Renderer::CullRenderCommands()
	{
		const uint32_t currentFrame = s_rendererData->commandBuffer->GetCurrentIndex();

		uint32_t* drawCount = s_rendererData->indirectCountBuffer->Get(currentFrame)->Map<uint32_t>();

		std::vector<IndirectBatch>& draws = s_rendererData->indirectBatches;

		for (uint32_t i = 0; i < draws.size(); i++)
		{
			drawCount[i] = 0;
		}

		s_rendererData->indirectCountBuffer->Get(currentFrame)->Unmap();

		s_rendererData->indirectCullPipeline->SetStorageBuffer(s_rendererData->indirectDrawBuffer, 0, 1, VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
		s_rendererData->indirectCullPipeline->SetStorageBuffer(s_rendererData->indirectCountBuffer, 0, 2, VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
		s_rendererData->indirectCullPipeline->SetStorageBuffer(ShaderStorageBufferRegistry::Get(1, 1), 0, 3, VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
		s_rendererData->indirectCullPipeline->SetStorageBuffer(ShaderStorageBufferRegistry::Get(1, 0), 0, 4, VK_ACCESS_INDIRECT_COMMAND_READ_BIT);

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
		s_rendererData->indirectCullPipeline->Dispatch(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), currentFrame, dispatchCount, 1, 1);
		s_rendererData->indirectCullPipeline->InsertBarrier(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), currentFrame, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);
	}
}