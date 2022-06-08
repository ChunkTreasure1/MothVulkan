#pragma once

#include <Lamp/Event/ApplicationEvent.h>
#include <Lamp/Core/Layer/Layer.h>

namespace Lamp
{
	class SceneRenderer;
}

class Scene;
class EditorWindow;
class EditorCameraController;

class Sandbox : public Lamp::Layer
{
public:
	Sandbox() = default;
	~Sandbox() override = default;

	void OnAttach() override;
	void OnDetach() override;

	void OnEvent(Lamp::Event& e) override;

private:
	bool OnImGuiUpdateEvent(Lamp::AppImGuiUpdateEvent& e);

	/////ImGui/////
	void UpdateDockSpace();
	///////////////

	std::vector<Ref<EditorWindow>> m_editorWindows;

	Ref<Lamp::SceneRenderer> m_sceneRenderer;
	Ref<Scene> m_editorScene;
	Ref<EditorCameraController> m_editorCameraController;
};
