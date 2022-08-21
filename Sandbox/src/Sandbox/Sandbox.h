#pragma once

#include "Sandbox/Utility/Helpers.h"
#include "Sandbox/VersionControl/VersionControl.h"

#include <Lamp/Scene/Entity.h>
#include <Lamp/Core/Layer/Layer.h>
#include <Lamp/Event/ApplicationEvent.h>
#include <Lamp/Event/KeyEvent.h>

#include <imgui.h>
#include <ImGuizmo.h>

namespace Lamp
{
	class SceneRenderer;
	class EditorCameraController;
	class Scene;
}

struct EditorSettings
{
	VersionControlSettings versionControlSettings;
};

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
	void NewScene();
	void OpenScene();
	void SaveScene();
	void SaveSceneAs();

	bool OnImGuiUpdateEvent(Lamp::AppImGuiUpdateEvent& e);
	bool OnRenderEvent(Lamp::AppRenderEvent& e);
	bool OnKeyPressedEvent(Lamp::KeyPressedEvent& e);

	/////ImGui/////
	void UpdateDockSpace();
	void ShouldSavePopup();
	///////////////

	std::vector<Ref<EditorWindow>> m_editorWindows;
	std::vector<Wire::EntityId> m_selectedEntities;

	Ref<Lamp::SceneRenderer> m_sceneRenderer;
	Lamp::EditorCameraController* m_editorCameraController;
	Ref<Lamp::Scene> m_editorScene;

	EditorSettings m_settings;

	ImGuizmo::OPERATION m_gizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
	ImGuizmo::MODE m_gizmoMode = ImGuizmo::MODE::WORLD;
};
