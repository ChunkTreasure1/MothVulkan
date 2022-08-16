#pragma once

#include "Sandbox/Window/EditorWindow.h"
#include "Sandbox/Window/AssetCommon.h"

namespace Lamp
{
	class Texture2D;
}

class AssetBrowserPanel : public EditorWindow
{
public:
	AssetBrowserPanel();

	void UpdateMainContent() override;

private:
	Ref<DirectoryData> ProcessDirectory(const std::filesystem::path& path, Ref<DirectoryData> parent);
	std::vector<DirectoryData*> FindParentDirectoriesOfDirectory(DirectoryData* directory);

	void RenderControlsBar(float height);
	void RenderDirectory(const Ref<DirectoryData> dirData);
	void RenderView(const std::vector<Ref<DirectoryData>>& dirData, const std::vector<AssetData>& assetData);
	void RenderFilePopup(const AssetData& data);
	void RenderFileInfo(const AssetData& data);
	void Reload();
	
	void Search(const std::string& query);
	void FindFoldersAndFilesWithQuery(const std::vector<Ref<DirectoryData>>& dirList, std::vector<Ref<DirectoryData>>& directories, std::vector<AssetData>& assets, const std::string& query);

	DirectoryData* FindDirectoryWithPath(const std::filesystem::path& path);
	DirectoryData* FindDirectoryWithPathRecursivly(const std::vector<Ref<DirectoryData>> dirList, const std::filesystem::path& path);

	std::unordered_map<std::string, Ref<DirectoryData>> m_directories;
	std::vector<DirectoryData*> m_directoryButtons;
	std::filesystem::path m_currentDirectoryPath;

	DirectoryData* m_currentDirectory = nullptr;
	DirectoryData* m_nextDirectory = nullptr;

	DirectoryData* m_engineDirectory = nullptr;
	DirectoryData* m_assetsDirectory = nullptr;

	float m_thumbnailPadding = 16.f;
	float m_thumbnailSize = 85.f;
	bool m_showEngineAssets = false;

	std::string m_searchQuery;
	std::vector<Ref<DirectoryData>> m_searchDirectories;
	std::vector<AssetData> m_searchAssets;
	bool m_hasSearchQuery = false;

	std::unordered_map<Lamp::AssetType, Ref<Lamp::Texture2D>> m_assetIcons;
};