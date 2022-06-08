#pragma once

#include "Lamp/Core/Base.h"

class Scene;

namespace Lamp
{
	class Camera;
	class SceneRenderer
	{
	public:
		SceneRenderer(Ref<Scene> scene);
		void OnRender(Ref<Camera> camera);

	private:
		Ref<Scene> m_scene;
	};
}