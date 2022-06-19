#pragma once

#include "Sandbox/Window/EditorWindow.h"

#include <Lamp/Asset/Asset.h>

#include <filesystem>

struct AssetData
{
	Lamp::AssetHandle handle;
	Lamp::AssetType type;
	std::filesystem::path path;
};

struct DirectoryData
{
	Lamp::AssetHandle handle;
	std::filesystem::path path;
	
	DirectoryData* parentDir;
	bool selected = false;

	std::vector<AssetData> assets;
	std::vector<Ref<DirectoryData>> subDirectories;
};

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

	float m_thumbnailPadding = 16.f;
	float m_thumbnailSize = 100.f;

	std::string m_searchQuery;
	std::vector<Ref<DirectoryData>> m_searchDirectories;
	std::vector<AssetData> m_searchAssets;
	bool m_hasSearchQuery = false;
};