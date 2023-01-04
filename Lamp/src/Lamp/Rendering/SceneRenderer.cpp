#include "lppch.h"
#include "SceneRenderer.h"

#include "Lamp/Asset/AssetManager.h"
#include "Lamp/Asset/Mesh/Mesh.h"

#include "Lamp/Rendering/Camera/Camera.h"
#include "Lamp/Rendering/Renderer.h"
#include "Lamp/Rendering/RenderGraph.h"
#include "Lamp/Rendering/RenderPass/RenderPass.h"
#include "Lamp/Rendering/Framebuffer.h"
#include "Lamp/Rendering/DependencyGraph.h"

#include "Lamp/Components/Components.h"
#include "Lamp/Scene/Scene.h"

#include <Wire/Registry.h>

namespace Lamp
{
	SceneRenderer::SceneRenderer(Ref<Scene> scene)
		: m_scene(scene)
	{
		CreateRenderPasses();
	}

	Ref<Framebuffer> SceneRenderer::GetFinalFramebuffer()
	{
		return m_mainRenderPass->framebuffer;
	}

	void SceneRenderer::CreateRenderPasses()
	{
		// Main
		{
			FramebufferSpecification spec{};
			spec.attachments = 
			{
				{ ImageFormat::RGBA, { 0.1f, 0.1f, 0.1f, 1.f }  },
				{ ImageFormat::DEPTH32F }
			};

			spec.width = 1280;
			spec.height = 720;

			m_mainRenderPass = CreateRef<RenderPass>();
			m_mainRenderPass->framebuffer = Framebuffer::Create(spec);
		}
	}

	void SceneRenderer::OnRender(Ref<Camera> camera)
	{
		LP_PROFILE_FUNCTION();

		if (m_shouldResize)
		{
			m_shouldResize = false;
			m_mainRenderPass->framebuffer->Resize(m_resizeSize.x, m_resizeSize.y);
		}

		auto& registry = m_scene->GetRegistry();

		registry.ForEach<MeshComponent, TransformComponent>([](Wire::EntityId id, const MeshComponent& meshComp, TransformComponent& transformComp)
			{
				if (meshComp.handle != Asset::Null() && transformComp.visible)
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
				if (transformComp.visible)
				{
					const glm::mat4 transform = glm::rotate(glm::mat4(1.f), glm::radians(transformComp.rotation.x), glm::vec3(1, 0, 0)) *
						glm::rotate(glm::mat4(1.f), glm::radians(transformComp.rotation.y), glm::vec3(0, 1, 0)) *
						glm::rotate(glm::mat4(1.f), glm::radians(transformComp.rotation.z), glm::vec3(0, 0, 1));

					Renderer::SubmitDirectionalLight(transform, dirLightComp.color, dirLightComp.intensity);
				}
			});

		registry.ForEach<EnvironmentComponent>([](Wire::EntityId id, EnvironmentComponent& envComp)
			{
				if (envComp.environmentHandle != envComp.lastEnvironmentHandle)
				{
					envComp.lastEnvironmentHandle = envComp.environmentHandle;
					envComp.currentSkybox = Renderer::GenerateEnvironmentMap(envComp.environmentHandle);
				}

				if (envComp.environmentHandle == Asset::Null())
				{
					Skybox emptySkybox{};
					emptySkybox.irradianceMap = Renderer::GetDefaultData().blackCubeImage;
					emptySkybox.radianceMap = Renderer::GetDefaultData().blackCubeImage;

					Renderer::SubmitEnvironment(emptySkybox);
				}
				else
				{
					Renderer::SubmitEnvironment(envComp.currentSkybox);
				}
			});

		{
			LP_PROFILE_SCOPE("SceneRenderer::Render");

			Renderer::Begin();

			Renderer::BeginPass(m_mainRenderPass, camera);
			Renderer::DispatchRenderCommandsTest();
			Renderer::EndPass();

			Renderer::End();
		}
	}

	void SceneRenderer::OnUpdate(float deltaTime)
	{
	}

	void SceneRenderer::Resize(uint32_t width, uint32_t height)
	{
		m_shouldResize = true;
		m_resizeSize = { width, height };
	}

	void SceneRenderer::SetScene(Ref<Scene> newScene)
	{
		m_scene = newScene;
	}
}
