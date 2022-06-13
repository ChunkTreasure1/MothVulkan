#pragma once

#include <Wire/SceneSystem/Entity.h>
#include <Lamp/Event/ApplicationEvent.h>
#include <Lamp/Core/Layer/Layer.h>

namespace Lamp
{
	class SceneRenderer;
	class EditorCameraController;
}

class Scene;
class EditorWindow;

class Sandbox : public Lamp::Layer
{
public:
	Sandbox() = default;
	~Sandbox() override;

	void OnAttach() override;
	void OnDetach() override;

	void OnEvent(Lamp::Event& e) override;

private:
	bool OnImGuiUpdateEvent(Lamp::AppImGuiUpdateEvent& e);
	bool OnRenderEvent(Lamp::AppRenderEvent& e);

	/////ImGui/////
	void UpdateDockSpace();
	///////////////

	std::vector<Ref<EditorWindow>> m_editorWindows;
	std::vector<Entity> m_selectedEntities;

	Ref<Lamp::SceneRenderer> m_sceneRenderer;
	Lamp::EditorCameraController* m_editorCameraController;
	Ref<Scene> m_editorScene;
};
