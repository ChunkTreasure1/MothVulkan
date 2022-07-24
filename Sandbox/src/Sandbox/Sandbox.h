#pragma once

#include <Lamp/Scene/Entity.h>
#include <Lamp/Core/Layer/Layer.h>
#include <Lamp/Event/ApplicationEvent.h>
#include <Lamp/Event/KeyEvent.h>

namespace Lamp
{
	class SceneRenderer;
	class EditorCameraController;
	class Scene;
}

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
	void ExecuteUndo();

	bool OnImGuiUpdateEvent(Lamp::AppImGuiUpdateEvent& e);
	bool OnRenderEvent(Lamp::AppRenderEvent& e);
	bool OnKeyPressedEvent(Lamp::KeyPressedEvent& e);

	/////ImGui/////
	void UpdateDockSpace();
	///////////////

	std::vector<Ref<EditorWindow>> m_editorWindows;
	std::vector<Lamp::Entity> m_selectedEntities;

	Ref<Lamp::SceneRenderer> m_sceneRenderer;
	Lamp::EditorCameraController* m_editorCameraController;
	Ref<Lamp::Scene> m_editorScene;
};
