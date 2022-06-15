#include "sbpch.h"
#include "ViewportPanel.h"

#include <Lamp/Rendering/Camera/EditorCameraController.h>

#include <Lamp/Rendering/SceneRenderer.h>
#include <Lamp/Rendering/Framebuffer.h>
#include <Lamp/Utility/UIUtility.h>

ViewportPanel::ViewportPanel(Ref<Lamp::SceneRenderer> sceneRenderer, Lamp::EditorCameraController* cameraController)
	: EditorWindow("Viewport"), m_sceneRenderer(sceneRenderer), m_editorCameraController(cameraController)
{
	m_isOpen = true;
	m_windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
}

void ViewportPanel::UpdateContent()
{
	auto size = ImGui::GetContentRegionAvail();
	
	auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
	auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
	auto viewportOffset = ImGui::GetWindowPos();
	
	m_perspectiveBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
	m_perspectiveBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };
	
	const float toolBarSize = 22.f;
	const float toolBarYPadding = 5.f;
	const float toolBarXPadding = 5.f;

	ImVec2 viewportSize = ImGui::GetContentRegionAvail();
	viewportSize.y -= (toolBarSize + toolBarYPadding);

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 0.f, 0.f });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0.f, 0.f });
	
	/////Perspective toolbar/////
	{
		UI::ScopedColor childBg(ImGuiCol_ChildBg, { 0.258f, 0.258f, 0.258f, 1.000f });
		ImGui::BeginChild("toolbarChild", { ImGui::GetContentRegionAvail().x, toolBarSize + toolBarYPadding }, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		UI::ShiftCursor(toolBarXPadding, toolBarYPadding / 2.f);

		UpdateToolbar(toolBarSize, toolBarXPadding);
		ImGui::EndChild();
	}
	/////////////////////////////

	ImGui::PopStyleVar(2);

	if (m_viewportSize != (*(glm::vec2*)&viewportSize))
	{
		m_viewportSize = { viewportSize.x, viewportSize.y };
		m_sceneRenderer->Resize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);

		m_editorCameraController->UpdateProjection((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
	}

	ImGui::Image(UI::GetTextureID(m_sceneRenderer->GetFinalFramebuffer()->GetColorAttachment(0)), size);
}

void ViewportPanel::UpdateToolbar(float toolbarHeight, float toolbarXPadding)
{
}
