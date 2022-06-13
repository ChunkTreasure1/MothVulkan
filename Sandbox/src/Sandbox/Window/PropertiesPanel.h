#pragma once

#include "Sandbox/Window/EditorWindow.h"

#include <Wire/SceneSystem/Entity.h>
#include <vector>

class PropertiesPanel : public EditorWindow
{
public:
	PropertiesPanel(std::vector<Entity>& selectedEntites);

	void UpdateContent() override;

private:
	std::vector<Entity>& m_selectedEntites;
};