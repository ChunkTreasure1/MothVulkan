#pragma once

#include "Sandbox/Window/EditorWindow.h"

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
	ViewportPanel(Ref<Lamp::SceneRenderer> sceneRenderer, Ref<Lamp::Scene>& editorScene, Lamp::EditorCameraController* cameraController);

	void UpdateMainContent() override;

private:
	void UpdateToolbar(float toolbarHeight, float toolbarXPadding);

	Ref<Lamp::SceneRenderer> m_sceneRenderer;
	Ref<Lamp::Scene>& m_editorScene;

	Lamp::EditorCameraController* m_editorCameraController;

	glm::vec2 m_perspectiveBounds[2] = { { 0.f, 0.f }, { 0.f, 0.f } };
	glm::vec2 m_viewportSize = { 1280.f, 720.f };
};