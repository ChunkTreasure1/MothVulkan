#pragma once

#include "Lamp/Core/Base.h"

#include <filesystem>


namespace Lamp
{
	class Scene;
	class Camera;
	class RenderGraph;
	class Framebuffer;
	class SceneRenderer
	{
	public:
		SceneRenderer(Ref<Scene> scene, const std::filesystem::path& renderGraphPath);

		void OnRender(Ref<Camera> camera);
		void OnUpdate(float deltaTime);

		void Resize(uint32_t width, uint32_t height);
		void SetScene(Ref<Scene> newScene);

		Ref<Framebuffer> GetFinalFramebuffer();

	private:

		Ref<RenderGraph> m_renderGraph;
		Ref<Scene> m_scene;
	};
}