#pragma once

#include "Lamp/Asset/Asset.h"

#include "Lamp/Log/Log.h"
#include "Lamp/Core/Base.h"

#include <unordered_map>
#include <filesystem>

namespace Lamp
{
	class AssetImporter;
	class AssetManager
	{
	public:
		AssetManager();
		~AssetManager();

		void Initialize();
		void Shutdown();

		void LoadAsset(const std::filesystem::path& path, Ref<Asset>& asset);
		void LoadAsset(AssetHandle assetHandle, Ref<Asset>& asset);
		void SaveAsset(const Ref<Asset> asset);
		void MoveAsset(Ref<Asset> asset, std::filesystem::path& targetDir);

		Ref<Asset> GetAssetRaw(AssetHandle assetHandle);

		AssetType GetAssetTypeFromHandle(const AssetHandle& handle);
		AssetType GetAssetTypeFromPath(const std::filesystem::path& path);
		AssetType GetAssetTypeFromExtension(const std::string& extension);
		AssetHandle GetAssetHandleFromPath(const std::filesystem::path& path);
		std::filesystem::path GetPathFromAssetHandle(AssetHandle handle);

		inline static AssetManager& Get() { return *s_instance; }

		template<typename T>
		static Ref<T> GetAsset(AssetHandle assetHandle);

		template<typename T>
		static AssetHandle GetHandle(const std::filesystem::path& path);

		template<typename T>
		static Ref<T> GetAsset(const std::filesystem::path& path);

	private:
		struct LoadJob
		{
			Ref<Asset>* asset;
			std::filesystem::path path;
		};

		inline static AssetManager* s_instance = nullptr;

		void SaveAssetRegistry();
		void LoadAssetRegistry();

		void Thread_LoadAsset();

		std::unordered_map<AssetType, Scope<AssetImporter>> m_assetImporters;
		std::unordered_map<std::filesystem::path, AssetHandle> m_assetRegistry;
		std::unordered_map<AssetHandle, Ref<Asset>> m_assetCache;

		std::thread m_loadThread;
		std::mutex m_loadMutex;
		std::atomic_bool m_isThreadRunning = false;
		std::condition_variable m_threadConditionVar;

		std::vector<LoadJob> m_loadQueue;
	};

	template<typename T>
	inline Ref<T> AssetManager::GetAsset(AssetHandle assetHandle)
	{
		if (assetHandle == Asset::Null())
		{
			return nullptr;
		}

		Ref<Asset> asset = CreateRef<T>();
		asset->SetFlag(AssetFlag::Queued, true);
		Get().LoadAsset(assetHandle, asset);

		return std::reinterpret_pointer_cast<T>(asset);
	}

	template<typename T>
	inline AssetHandle AssetManager::GetHandle(const std::filesystem::path& path)
	{
		if (!std::filesystem::exists(path))
		{
			LP_CORE_ERROR("Unable to load asset {0}! It does not exist!", path.string().c_str());
			return Asset::Null();
		}

		auto it = Get().m_assetRegistry.find(path);
		if (it != Get().m_assetRegistry.end())
		{
			return it->second;
		}
	
		Ref<T> asset = GetAsset<T>(path);
		if (asset)
		{
			return asset->handle;
		}

		return Asset::Null();
	}

	template<typename T>
	inline Ref<T> AssetManager::GetAsset(const std::filesystem::path& path)
	{
		if (!std::filesystem::exists(path))
		{
			LP_CORE_ERROR("Unable to load asset {0}! It does not exist!", path.string().c_str());
			return nullptr;
		}

		Ref<Asset> asset = CreateRef<T>();
		asset->SetFlag(AssetFlag::Queued, true);
		Get().LoadAsset(path, asset);

		return std::reinterpret_pointer_cast<T>(asset);
	}
}