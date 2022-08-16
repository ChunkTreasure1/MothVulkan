#pragma once

#include "Sandbox/Window/EditorWindow.h"

struct EditorSettings;

class EditorSettingsPanel : public EditorWindow
{
public:
	EditorSettingsPanel(EditorSettings& settings);

	void UpdateMainContent();

private:
	EditorSettings& m_editorSettings;
};