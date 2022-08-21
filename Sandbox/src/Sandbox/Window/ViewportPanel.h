#pragma once

#include "Sandbox/Window/EditorWindow.h"

#include <Wire/Entity.h>

#include <imgui.h>
#include <ImGuizmo.h>

namespace Lamp
{
	class Framebuffer;
	class SceneRenderer;
	class EditorCameraController;
	class Scene;
}

class ViewportPanel : public EditorWindow
{
public:
	ViewportPanel(Ref<Lamp::SceneRenderer> sceneRenderer, Ref<Lamp::Scene>& editorScene, Lamp::EditorCameraController* cameraController, std::vector<Wire::EntityId>& selectedEntites, 
		ImGuizmo::OPERATION& gizmoOperation, ImGuizmo::MODE& gizmoMode);

	void UpdateMainContent() override;

private:
	void UpdateToolbar(float toolbarHeight, float toolbarXPadding);

	Ref<Lamp::SceneRenderer> m_sceneRenderer;
	Ref<Lamp::Scene>& m_editorScene;
	std::vector<Wire::EntityId>& m_selectedEntites;

	Lamp::EditorCameraController* m_editorCameraController;

	glm::vec2 m_perspectiveBounds[2] = { { 0.f, 0.f }, { 0.f, 0.f } };
	glm::vec2 m_viewportSize = { 1280.f, 720.f };

	ImGuizmo::OPERATION& m_gizmoOperation;
	ImGuizmo::MODE& m_gizmoMode;
};