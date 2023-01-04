#pragma once

#include "Lamp/Core/Base.h"

#include <filesystem>


namespace Lamp
{
	class Scene;
	class Camera;
	class Framebuffer;
	class RenderPass;

	class SceneRenderer
	{
	public:
		SceneRenderer(Ref<Scene> scene);

		void OnRender(Ref<Camera> camera);
		void OnUpdate(float deltaTime);

		void Resize(uint32_t width, uint32_t height);
		void SetScene(Ref<Scene> newScene);

		Ref<Framebuffer> GetFinalFramebuffer();

	private:
		void CreateRenderPasses();

		bool m_shouldResize = false;
		glm::uvec2 m_resizeSize = { 1, 1 };

		Ref<Scene> m_scene;

		Ref<RenderPass> m_mainRenderPass;
	};
}