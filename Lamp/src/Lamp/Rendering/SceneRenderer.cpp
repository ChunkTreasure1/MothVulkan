#include "lppch.h"
#include "SceneRenderer.h"

#include "Lamp/Asset/AssetManager.h"
#include "Lamp/Asset/Mesh/Mesh.h"

#include "Lamp/Rendering/Camera/Camera.h"
#include "Lamp/Rendering/Renderer.h"
#include "Lamp/Rendering/RenderGraph.h"
#include "Lamp/Rendering/RenderPass/RenderPass.h"
#include "Lamp/Rendering/Framebuffer.h"

#include "Lamp/Components/Components.h"

#include <Wire/SceneSystem/Scene.h>
#include <Wire/ECS/Registry.h>

namespace Lamp
{
	SceneRenderer::SceneRenderer(Ref<Scene> scene, const std::filesystem::path& renderGraphPath)
		: m_scene(scene)
	{
		m_renderGraph = AssetManager::GetAsset<RenderGraph>(renderGraphPath);
	}

	Ref<Framebuffer> SceneRenderer::GetFinalFramebuffer()
	{
		return m_renderGraph->GetRenderPasses().back().renderPass->framebuffer;
	}

	void SceneRenderer::OnRender(Ref<Camera> camera)
	{
		LP_PROFILE_FUNCTION();

		auto& registry = m_scene->GetRegistry();

		registry.ForEach<MeshComponent, TransformComponent>([](Wire::EntityId id, const MeshComponent& meshComp, TransformComponent& transformComp)
			{
				if (meshComp.handle != Asset::Null())
				{
					auto mesh = AssetManager::GetAsset<Mesh>(meshComp.handle);
					const glm::mat4 transform = glm::translate(glm::mat4(1.f), transformComp.position) *
						glm::rotate(glm::mat4(1.f), glm::radians(transformComp.rotation.x), glm::vec3(1, 0, 0)) *
						glm::rotate(glm::mat4(1.f), glm::radians(transformComp.rotation.y), glm::vec3(0, 1, 0)) *
						glm::rotate(glm::mat4(1.f), glm::radians(transformComp.rotation.z), glm::vec3(0, 0, 1)) *
						glm::scale(glm::mat4(1.f), transformComp.scale);

					Renderer::Submit(mesh, transform);
				}
			});

		registry.ForEach<DirectionalLightComponent, TransformComponent>([](Wire::EntityId id, const DirectionalLightComponent& dirLightComp, const TransformComponent& transformComp)
			{
				const glm::mat4 transform = glm::rotate(glm::mat4(1.f), glm::radians(transformComp.rotation.x), glm::vec3(1, 0, 0)) *
					glm::rotate(glm::mat4(1.f), glm::radians(transformComp.rotation.y), glm::vec3(0, 1, 0)) *
					glm::rotate(glm::mat4(1.f), glm::radians(transformComp.rotation.z), glm::vec3(0, 0, 1));

				Renderer::SubmitDirectionalLight(transform, dirLightComp.color, dirLightComp.intensity);
			});

		{
			LP_PROFILE_SCOPE("SceneRenderer::Render");

			Renderer::Begin();

			for (const auto& pass : m_renderGraph->GetRenderPasses())
			{
				Renderer::BeginPass(pass.renderPass, camera);
				Renderer::DispatchRenderCommands();
				Renderer::EndPass();
			}

			Renderer::End();
		}
	}

	void SceneRenderer::Resize(uint32_t width, uint32_t height)
	{
		for (auto& pass : m_renderGraph->GetRenderPasses())
		{
			if (pass.renderPass->resizeable)
			{
				pass.renderPass->framebuffer->Resize(width, height);
			}
		}
	}
}
