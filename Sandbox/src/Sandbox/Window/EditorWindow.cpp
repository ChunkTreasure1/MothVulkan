#include "sbpch.h"
#include "EditorWindow.h"

EditorWindow::EditorWindow(const std::string& title)
	: m_title(title)
{}

bool EditorWindow::Begin()
{
	if (!m_isOpen)
	{
		return false;
	}

	ImGui::Begin(m_title.c_str(), &m_isOpen, m_windowFlags);
	m_isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	m_isHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

	return true;
}

void EditorWindow::End()
{
	ImGui::End();
}
