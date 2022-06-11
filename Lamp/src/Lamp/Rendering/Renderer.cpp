#include "lppch.h"
#include "Renderer.h"

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

#include "Lamp/Rendering/Buffer/IndexBuffer.h"
#include "Lamp/Rendering/Buffer/VertexBuffer.h"
#include "Lamp/Rendering/Camera/Camera.h"
#include "Lamp/Rendering/Texture/Texture2D.h"

#include "Lamp/Asset/Mesh/Material.h"
#include "Lamp/Asset/Mesh/Mesh.h"

namespace Lamp
{
	void Renderer::Initialize()
	{
		s_rendererData = CreateScope<RendererData>();
		s_defaultData = CreateScope<DefaultData>();

		const uint32_t framesInFlight = Application::Get().GetWindow()->GetSwapchain().GetFramesInFlight();
		s_rendererData->commandBuffer = CommandBuffer::Create(framesInFlight, false);

		s_rendererData->indirectDrawBuffer = ShaderStorageBufferSet::Create(sizeof(VkDrawIndexedIndirectCommand) * 100, framesInFlight, true);

		CreateDefaultData();
		CreateDescriptorPools();
	}

	void Renderer::Shutdowm()
	{
		for (uint32_t i = 0; i < s_rendererData->descriptorPools.size(); i++)
		{
			vkDestroyDescriptorPool(GraphicsContext::GetDevice()->GetHandle(), s_rendererData->descriptorPools[i], nullptr);
		}
		
		s_rendererData = nullptr;
		s_defaultData = nullptr;

	}

	void Renderer::Begin()
	{
		LP_PROFILE_FUNCTION();

		uint32_t currentFrame = Application::Get().GetWindow()->GetSwapchain().GetCurrentFrame();
		LP_VK_CHECK(vkResetDescriptorPool(GraphicsContext::GetDevice()->GetHandle(), s_rendererData->descriptorPools[currentFrame], 0));

		SortRenderCommands();
		UpdatePerFrameBuffers();

		s_rendererData->commandBuffer->Begin();
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
			cmd.material = mesh->GetMaterials().at(subMesh.materialIndex);
			cmd.subMesh = subMesh;
			cmd.transform = transform;
		}
	}

	void Renderer::Draw()
	{
		LP_PROFILE_FUNCTION();
		if (s_rendererData->renderCommands.empty())
		{
			return;
		}

		const uint32_t currentFrame = s_rendererData->commandBuffer->GetCurrentIndex();

		// Draw
		{
			std::vector<IndirectBatch> draws = PrepareForIndirectDraw(s_rendererData->renderCommands);

			VkDrawIndexedIndirectCommand* drawCommands = s_rendererData->indirectDrawBuffer->Get(currentFrame)->Map<VkDrawIndexedIndirectCommand>();

			for (uint32_t i = 0; i < draws.size(); i++)
			{
				drawCommands[i].indexCount = draws[i].subMesh.indexCount;
				drawCommands[i].firstIndex = draws[i].subMesh.indexStartOffset;
				drawCommands[i].vertexOffset = draws[i].subMesh.vertexStartOffset;
				drawCommands[i].instanceCount = draws[i].count;
				drawCommands[i].firstInstance = 0;
			}

			s_rendererData->indirectDrawBuffer->Get(currentFrame)->Unmap();

			draws.front().material->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), currentFrame);
			for (uint32_t i = 0; i < draws.size(); i++)
			{
				if (i > 0 && draws[i].material != draws[i - 1].material)
				{
					draws[i].material->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), currentFrame);
				}

				draws[i].mesh->GetVertexBuffer()->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer());
				draws[i].mesh->GetIndexBuffer()->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer());

				VkDeviceSize drawOffset = draws[i].first * sizeof(VkDrawIndexedIndirectCommand);
				uint32_t drawStride = sizeof(VkDrawIndexedIndirectCommand);

				draws[i].material->SetPushConstant(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), 0, sizeof(uint32_t), &i);
				vkCmdDrawIndexedIndirect(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), s_rendererData->indirectDrawBuffer->Get(currentFrame)->GetHandle(), drawOffset, 1, drawStride);
			}
		}
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

	std::vector<IndirectBatch> Renderer::PrepareForIndirectDraw(std::vector<RenderCommand>& renderCommands)
	{
		LP_PROFILE_FUNCTION();

		std::vector<IndirectBatch> draws;

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
				newDraw.first = draws.size() - 1;
				newDraw.count = 1;
			}
		}

		std::sort(draws.begin(), draws.end(), [](const IndirectBatch& lhs, const IndirectBatch& rhs)
			{
				return lhs.material < rhs.material;
			});

		return draws;
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
				objectData[i].transform = s_rendererData->renderCommands[i].transform;
			}

			currentObjectBuffer->Unmap();
		}
	}
	
	void Renderer::SortRenderCommands()
	{
		std::sort(s_rendererData->renderCommands.begin(), s_rendererData->renderCommands.end(), [](const RenderCommand& lhs, const RenderCommand& rhs)
		{
			return lhs.subMesh > rhs.subMesh || lhs.mesh > rhs.mesh;
		});
	}
}