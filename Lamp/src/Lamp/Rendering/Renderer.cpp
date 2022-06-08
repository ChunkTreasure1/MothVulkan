#include "lppch.h"
#include "Renderer.h"

#include "Lamp/Core/Application.h"
#include "Lamp/Core/Window.h"
#include "Lamp/Core/Graphics/Swapchain.h"

#include "Lamp/Log/Log.h"

#include "Lamp/Rendering/Buffer/CommandBuffer.h"
#include "Lamp/Rendering/RenderPass/RenderPass.h"
#include "Lamp/Rendering/Framebuffer.h"

#include "Lamp/Rendering/Buffer/ShaderStorageBuffer/ShaderStorageBufferRegistry.h"
#include "Lamp/Rendering/Buffer/ShaderStorageBuffer/ShaderStorageBufferSet.h"

#include "Lamp/Rendering/Buffer/UniformBuffer/UniformBufferRegistry.h"
#include "Lamp/Rendering/Buffer/UniformBuffer/UniformBufferSet.h"

#include "Lamp/Rendering/Texture/Texture2D.h"




#include "Lamp/Asset/AssetManager.h"
#include "Lamp/Asset/Mesh/Material.h"
#include "Lamp/Asset/Mesh/Mesh.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipelineRegistry.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipeline.h"
#include "Lamp/Rendering/Buffer/IndexBuffer.h"
#include "Lamp/Rendering/Buffer/VertexBuffer.h"

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

		/////TESTING//////
		s_rendererData->mesh = AssetManager::GetAsset<Mesh>("Assets/SM_Particle_Chest.fbx");
		s_rendererData->texture = AssetManager::GetAsset<Texture2D>("Assets/Textures/peter_lambert.png");

		s_rendererData->renderPipeline = RenderPipelineRegistry::Get("trimesh");
	}

	void Renderer::Shutdowm()
	{
		s_rendererData = nullptr;
		s_defaultData = nullptr;
	}

	void Renderer::Begin()
	{
		Submit(s_rendererData->mesh, glm::mat4(1.f));
		s_rendererData->commandBuffer->Begin();
	}

	void Renderer::End()
	{
		s_rendererData->commandBuffer->End();
		s_rendererData->renderCommands.clear();
	}

	void Renderer::BeginPass(Ref<RenderPass> renderPass)
	{
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
		vkCmdEndRendering(s_rendererData->commandBuffer->GetCurrentCommandBuffer());
		s_rendererData->currentFramebuffer->Unbind(s_rendererData->commandBuffer->GetCurrentCommandBuffer());
		s_rendererData->currentFramebuffer = nullptr;
	}

	void Renderer::Submit(Ref<Mesh> mesh, const glm::mat4& transform)
	{
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
		if (s_rendererData->renderCommands.empty())
		{
			return;
		}

		const uint32_t currentFrame = s_rendererData->commandBuffer->GetCurrentIndex();

		const glm::vec3 camPos = { 0.f, 0.f, -2.f };
		const glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
		glm::mat4 projection = glm::perspective(glm::radians(70.f), 1280.f / 720.f, 0.1f, 1000.f);

		const glm::mat4 model = glm::scale(glm::mat4(1.f), glm::vec3(0.01f));
		const glm::mat4 transform = model;

		// Update camera data
		{
			auto currentCameraBuffer = UniformBufferRegistry::Get(0, 0)->Get(currentFrame);
			CameraData* cameraData = currentCameraBuffer->Map<CameraData>();

			cameraData->proj = projection;
			cameraData->view = view;
			cameraData->viewProj = projection * view;

			currentCameraBuffer->Unmap();
		}

		// Update object data
		{
			auto currentObjectBuffer = ShaderStorageBufferRegistry::Get(1, 0)->Get(currentFrame);
			ObjectData* objectData = currentObjectBuffer->Map<ObjectData>();

			for (uint32_t i = 0; i < s_rendererData->renderCommands.size(); i++) // TODO: change to single memcpy
			{
				objectData[i].transform = s_rendererData->renderCommands[i].transform;
			}

			currentObjectBuffer->Unmap();
		}


		// Draw
		{
			std::vector<IndirectBatch> draws = PrepareForIndirectDraw(s_rendererData->renderCommands);

			VkDrawIndexedIndirectCommand* drawCommands = s_rendererData->indirectDrawBuffer->Get(currentFrame)->Map<VkDrawIndexedIndirectCommand>();
			for (uint32_t i = 0; i < draws.size(); i++)
			{
				drawCommands[i].indexCount = draws[i].subMesh.indexCount;
				drawCommands[i].firstIndex = draws[i].subMesh.indexStartOffset;
				drawCommands[i].vertexOffset = draws[i].subMesh.vertexStartOffset;
				drawCommands[i].instanceCount = 1;
				drawCommands[i].firstInstance = i;
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

				vkCmdDrawIndexedIndirect(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), s_rendererData->indirectDrawBuffer->Get(currentFrame)->GetHandle(), drawOffset, draws[i].count, drawStride);
			}
		}
	}

	void Renderer::TEST_RecompileShader()
	{
		s_rendererData->renderPipeline->GetSpecification().shader->Reload(true);
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

	std::vector<IndirectBatch> Renderer::PrepareForIndirectDraw(const std::vector<RenderCommand>& renderCommands)
	{
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
				newDraw.first = i;
				newDraw.count = 1;
			}
		}

		std::sort(draws.begin(), draws.end(), [](const IndirectBatch& lhs, const IndirectBatch& rhs) 
			{
				return lhs.material < rhs.material;
			});

		return draws;
	}
}