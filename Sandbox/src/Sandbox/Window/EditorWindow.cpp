#include "sbpch.h"
#include "EditorWindow.h"

EditorWindow::EditorWindow(const std::string& title, bool dockSpace)
	: m_title(title), m_hasDockSpace(dockSpace)
{}

bool EditorWindow::Begin()
{
	if (!m_isOpen)
	{
		return false;
	}

	if (m_hasDockSpace)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.f, 0.f });
	}

	ImGui::Begin(m_title.c_str(), &m_isOpen, m_windowFlags);

	if (m_hasDockSpace)
	{
		ImGui::PopStyleVar();

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID(m_title.c_str());
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
		}
	}

	m_isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	m_isHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

	return true;
}

void EditorWindow::End()
{
	ImGui::End();
}

void EditorWindow::Open()
{
	m_isOpen = true;
}
