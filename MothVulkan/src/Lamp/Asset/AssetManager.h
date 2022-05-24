#pragma once

#include "Lamp/Asset/Asset.h"

#include <unordered_map>
#include <filesystem>

namespace Lamp
{
	class AssetManager
	{
	public:
		AssetManager();
		~AssetManager();
		
		void Initialize();
		void Shutdown();

		AssetType GetAssetTypeFromPath(const std::filesystem::path& path);
		AssetType GetAssetTypeFromExtension(const std::string& extension);
		AssetHandle GetAssetHandleFromPath(const std::filesystem::path& path);
		std::filesystem::path GetPathFromAssetHandle(AssetHandle handle);

		inline AssetManager& Get() { return *s_instance; }

	private:
		inline static AssetManager* s_instance = nullptr;

		void SaveAssetRegistry();
		void LoadAssetRegistry();

		//std::unordered_map<AssetType, Scope<>>
		std::unordered_map<std::filesystem::path, AssetHandle> m_assetRegistry;
	};
}