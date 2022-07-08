#pragma once

#include "Sandbox/Window/EditorWindow.h"

#include <Lamp/Scene/Entity.h>
#include <Lamp/Scene/Scene.h>

#include <vector>

class CreatePanel : public EditorWindow
{
public:
	CreatePanel(std::vector<Lamp::Entity>& selectedEntites, Ref<Lamp::Scene>& scene);
	void UpdateMainContent() override;

private:
	std::vector<Lamp::Entity>& m_selectedEntites;

	Ref<Lamp::Scene>& m_scene;

	bool m_brushListOpen = false;
	const float m_buttonHeight = 24.f;
};