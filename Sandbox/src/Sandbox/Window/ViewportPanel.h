#pragma once

#include "Sandbox/Window/EditorWindow.h"

class ViewportPanel : public EditorWindow
{
public:
	ViewportPanel();

	void UpdateContent() override;
};