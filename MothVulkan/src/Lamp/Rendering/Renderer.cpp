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




#include "Lamp/Asset/AssetManager.h"
#include "Lamp/Asset/Mesh/MaterialInstance.h"
#include "Lamp/Asset/Mesh/Material.h"
#include "Lamp/Asset/Mesh/Mesh.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipelineRegistry.h"
#include "Lamp/Rendering/Buffer/IndexBuffer.h"
#include "Lamp/Rendering/Buffer/VertexBuffer.h"

namespace Lamp
{
	void Renderer::Initialize()
	{
		s_rendererData = CreateScope<RendererData>();

		const uint32_t framesInFlight = Application::Get().GetWindow()->GetSwapchain().GetFramesInFlight();
		s_rendererData->commandBuffer = CommandBuffer::Create(framesInFlight, false);

		/////TESTING//////
		s_rendererData->mesh = AssetManager::GetAsset<Mesh>("Assets/SM_Particle_Chest.fbx");
		s_rendererData->texture = AssetManager::GetAsset<Texture2D>("Assets/Textures/peter_lambert.png");

		s_rendererData->renderPipeline = RenderPipelineRegistry::Get("trimesh");

		s_rendererData->material = Material::Create("Mat", 0, s_rendererData->renderPipeline);
		s_rendererData->material->SetTexture(0, s_rendererData->texture);

		s_rendererData->materialInstance = MaterialInstance::Create(s_rendererData->material);
	}

	void Renderer::Shutdowm()
	{
		s_rendererData = nullptr;
	}

	void Renderer::Begin()
	{
		s_rendererData->commandBuffer->Begin();
	}

	void Renderer::End()
	{
		s_rendererData->commandBuffer->End();
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
			renderingInfo.renderArea = { 0, 0, framebuffer->GetSpecification().width, framebuffer->GetSpecification().height };
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

	void Renderer::Draw()
	{
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

			objectData[0].transform = transform;

			currentObjectBuffer->Unmap();
		}


		// Draw
		{
			s_rendererData->materialInstance->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), currentFrame);
			s_rendererData->mesh->GetVertexBuffer()->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer());
			s_rendererData->mesh->GetIndexBuffer()->Bind(s_rendererData->commandBuffer->GetCurrentCommandBuffer());

			for (auto& submesh : s_rendererData->mesh->GetSubMeshes())
			{
				vkCmdDrawIndexed(s_rendererData->commandBuffer->GetCurrentCommandBuffer(), submesh.indexCount, 1, submesh.indexStartOffset, submesh.vertexStartOffset, 0);
			}
		}
	}
}