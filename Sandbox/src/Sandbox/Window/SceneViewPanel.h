#pragma once

#include "Sandbox/Window/EditorWindow.h"
#include "Sandbox/Utility/Helpers.h"

#include <Lamp/Scene/Entity.h>
#include <Lamp/Scene/Scene.h>

#include <vector>

class SceneViewPanel : public EditorWindow
{
public:
	SceneViewPanel(std::vector<Lamp::Entity>& selectedEntities, Ref<Lamp::Scene>& scene);
	void UpdateMainContent() override;

private:
	void RenderSearchBar();
	void Search(const std::string query);

	std::string m_searchQuery;
	bool m_hasSearchQuery = false;

	std::vector<Wire::EntityId> m_entities;
	std::vector<Lamp::Entity>& m_selectedEntities;

	Ref<Lamp::Scene>& m_scene;
};