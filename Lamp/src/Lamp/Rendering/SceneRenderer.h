#pragma once

#include "Lamp/Core/Base.h"

#include <filesystem>

class Scene;

namespace Lamp
{
	class Camera;
	class SceneRenderer
	{
	public:
		SceneRenderer(Ref<Scene> scene, const std::filesystem::path& renderGraphPath);
		void OnRender(Ref<Camera> camera);

	private:

		
		Ref<Scene> m_scene;
	};
}