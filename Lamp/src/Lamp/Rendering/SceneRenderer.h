#pragma once

#include "Lamp/Core/Base.h"

#include <filesystem>

class Scene;

namespace Lamp
{
	class Camera;
	class RenderGraph;
	class Framebuffer;
	class SceneRenderer
	{
	public:
		SceneRenderer(Ref<Scene> scene, const std::filesystem::path& renderGraphPath);
		void OnRender(Ref<Camera> camera);

		Ref<Framebuffer> GetFinalFramebuffer();

	private:

		Ref<RenderGraph> m_renderGraph;
		Ref<Scene> m_scene;
	};
}