#pragma once

#include "Sandbox/Window/EditorWindow.h"

namespace Lamp
{
	class Framebuffer;
	class SceneRenderer;
}

class ViewportPanel : public EditorWindow
{
public:
	ViewportPanel(Ref<Lamp::SceneRenderer> sceneRenderer);

	void UpdateContent() override;

private:
	void UpdateToolbar(float toolbarHeight, float toolbarXPadding);

	Ref<Lamp::SceneRenderer> m_sceneRenderer;

	glm::vec2 m_perspectiveBounds[2] = { { 0.f, 0.f }, { 0.f, 0.f } };
	glm::vec2 m_viewportSize = { 1280.f, 720.f };
};