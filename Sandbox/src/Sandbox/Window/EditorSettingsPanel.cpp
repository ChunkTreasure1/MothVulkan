#include "sbpch.h"
#include "EditorSettingsPanel.h"

#include "Sandbox/Sandbox.h"
#include "Sandbox/VersionControl/VersionControl.h"

#include <imgui_stdlib.h>

EditorSettingsPanel::EditorSettingsPanel(EditorSettings& settings)
	: EditorWindow("Editor Settings"), m_editorSettings(settings)
{
}

void EditorSettingsPanel::UpdateMainContent()
{
	ImGui::InputTextString("Host", &m_editorSettings.versionControlSettings.host);
	ImGui::InputTextString("Port", &m_editorSettings.versionControlSettings.port);
	ImGui::InputTextString("User", &m_editorSettings.versionControlSettings.user);
	ImGui::InputTextString("Password", &m_editorSettings.versionControlSettings.password);

	if (ImGui::Button("Connect"))
	{
		VersionControl::Connect(m_editorSettings.versionControlSettings.host, m_editorSettings.versionControlSettings.port, m_editorSettings.versionControlSettings.user, m_editorSettings.versionControlSettings.password);
	}
}
