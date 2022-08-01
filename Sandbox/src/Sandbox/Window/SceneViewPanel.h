#pragma once

#include "Sandbox/Window/EditorWindow.h"

#include <Lamp/Scene/Entity.h>
#include <Lamp/Scene/Scene.h>

#include <vector>

class SceneViewPanel : public EditorWindow
{
public:
	SceneViewPanel(std::vector<Lamp::Entity>& selectedEntities, Ref<Lamp::Scene>& scene);
	void UpdateMainContent() override;

private:
	std::vector<Lamp::Entity>& m_selectedEntities;
	Ref<Lamp::Scene>& m_scene;
};