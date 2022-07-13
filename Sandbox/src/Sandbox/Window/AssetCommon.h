#pragma once

#include <Lamp/Asset/Asset.h>

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