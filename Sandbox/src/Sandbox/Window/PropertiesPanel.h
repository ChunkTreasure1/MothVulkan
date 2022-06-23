#pragma once

#include "Sandbox/Window/EditorWindow.h"

#include <Lamp/Scene/Entity.h>

#include <vector>

class PropertiesPanel : public EditorWindow
{
public:
	PropertiesPanel(std::vector<Lamp::Entity>& selectedEntites);

	void UpdateMainContent() override;

private:
	std::vector<Lamp::Entity>& m_selectedEntites;
};