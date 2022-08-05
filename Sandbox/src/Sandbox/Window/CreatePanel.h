#pragma once

#include "Sandbox/Window/EditorWindow.h"
#include "Sandbox/Window/AssetCommon.h"

#include <Lamp/Scene/Entity.h>
#include <Lamp/Scene/Scene.h>

#include <vector>

class CreatePanel : public EditorWindow
{
public:
	CreatePanel(std::vector<Wire::EntityId>& selectedEntites, Ref<Lamp::Scene>& scene);
	void UpdateMainContent() override;

private:
	std::shared_ptr<DirectoryData> ProcessDirectory(const std::filesystem::path& path, std::shared_ptr<DirectoryData> parent);
	void RenderDirectory(const std::shared_ptr<DirectoryData> dirData, uint32_t& i);

	std::vector<Wire::EntityId>& m_selectedEntites;

	Ref<Lamp::Scene>& m_scene;

	std::unordered_map<std::string, Ref<DirectoryData>> m_directories;
	DirectoryData* m_currentDirectory = nullptr;

	bool m_meshListOpen = false;
	bool m_hasSearchQuery = false;
	const float m_buttonSize = 24.f;

	std::string m_searchQuery;
};