#pragma once

#include "Sandbox/Window/EditorWindow.h"

#include <Lamp/Scene/Entity.h>
#include <Lamp/Scene/Scene.h>

#include <vector>

class PropertiesPanel : public EditorWindow
{
public:
	PropertiesPanel(std::vector<Lamp::Entity>& selectedEntites, Ref<Lamp::Scene>& currentScene);
	void UpdateMainContent() override;

private:
	void AddComponentPopup();

	std::vector<Lamp::Entity>& m_selectedEntites;
	Ref<Lamp::Scene>& m_currentScene;
};