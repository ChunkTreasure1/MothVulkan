#include "lppch.h"
#include "AssetManager.h"

#include "Lamp/Asset/Importers/AssetImporter.h"
#include "Lamp/Asset/Importers/MeshImporter.h"
#include "Lamp/Asset/Importers/MeshSourceImporter.h"

#include "Lamp/Core/Base.h"
#include "Lamp/Log/Log.h"

#include "Lamp/Utility/StringUtility.h"

#include <yaml-cpp/yaml.h>

namespace Lamp
{
	static const std::filesystem::path s_assetRegistryPath = "Assets/AssetRegistry.lpreg";

	AssetManager::AssetManager()
	{
		LP_CORE_ASSERT(!s_instance, "AssetManager already exists!");
		s_instance = this;

		Initialize();
	}

	AssetManager::~AssetManager()
	{
		Shutdown();
		s_instance = nullptr;
	}

	void AssetManager::Initialize()
	{
		MeshImporter::Initialize();

		m_assetImporters.emplace(AssetType::MeshSource, CreateScope<MeshSourceImporter>());
		LoadAssetRegistry();
	}

	void AssetManager::Shutdown()
	{
		SaveAssetRegistry();
		MeshImporter::Shutdown();
	}

	void AssetManager::LoadAsset(const std::filesystem::path& path, Ref<Asset>& asset)
	{
		AssetHandle handle = Asset::Null();
		if (m_assetRegistry.find(path) != m_assetRegistry.end())
		{
			handle = m_assetRegistry.at(path);
		}

		if (handle != Asset::Null() && m_assetCache.find(handle) != m_assetCache.end())
		{
			asset = m_assetCache[handle];
			return;
		}

		const auto type = GetAssetTypeFromPath(path);

		if (m_assetImporters.find(type) == m_assetImporters.end())
		{
			LP_CORE_ERROR("No importer for asset found!");
			return;
		}

		m_assetImporters[type]->Load(path, asset);
		if (handle != Asset::Null())
		{
			asset->handle = handle;
		}
		else
		{
			m_assetRegistry.emplace(path, asset->handle);
		}

		asset->path = path;
		m_assetCache.emplace(asset->handle, asset);
	}

	void AssetManager::LoadAsset(AssetHandle assetHandle, Ref<Asset>& asset)
	{
		if (m_assetCache.find(assetHandle) != m_assetCache.end())
		{
			asset = m_assetCache[assetHandle];
			return;
		}

		const auto path = GetPathFromAssetHandle(assetHandle);
		if (!path.empty())
		{
			LoadAsset(path, asset);
		}
	}

	void AssetManager::SaveAsset(const Ref<Asset> asset)
	{
		if (m_assetImporters.find(asset->GetType()) == m_assetImporters.end())
		{
			LP_CORE_ERROR("No exporter for asset found!");
			return;
		}

		if (!asset->IsValid())
		{
			LP_CORE_ERROR("Unable to save invalid asset!");
			return;
		}

		if (m_assetRegistry.find(asset->path) == m_assetRegistry.end())
		{
			m_assetRegistry.emplace(asset->path, asset->handle);
		}

		if (m_assetCache.find(asset->handle) == m_assetCache.end())
		{
			m_assetCache.emplace(asset->handle, asset);
		}

		m_assetImporters[asset->GetType()]->Save(asset);
	}

	AssetType AssetManager::GetAssetTypeFromPath(const std::filesystem::path& path)
	{
		return GetAssetTypeFromExtension(path.extension().string());
	}

	AssetType AssetManager::GetAssetTypeFromExtension(const std::string& extension)
	{
		std::string ext = Utility::ToLower(extension);
		if (s_assetExtensionsMap.find(ext) == s_assetExtensionsMap.end()) [[unlikely]]
		{
			return AssetType::None;
		}

		return s_assetExtensionsMap.at(ext);
	}

	AssetHandle AssetManager::GetAssetHandleFromPath(const std::filesystem::path& path)
	{
		return m_assetRegistry.find(path) != m_assetRegistry.end() ? m_assetRegistry[path] : Asset::Null();
	}

	std::filesystem::path AssetManager::GetPathFromAssetHandle(AssetHandle handle)
	{
		for (const auto& [path, asset] : m_assetRegistry)
		{
			if (asset == handle)
			{
				return path;
			}
		}

		return "";
	}

	void AssetManager::SaveAssetRegistry()
	{
		YAML::Emitter out;
		out << YAML::BeginMap;

		out << YAML::Key << "Assets" << YAML::BeginSeq;
		for (const auto& [path, handle] : m_assetRegistry)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Handle" << YAML::Value << handle;
			out << YAML::Key << "Path" << YAML::Value << path.string();
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;

		std::ofstream fout(s_assetRegistryPath);
		fout << out.c_str();
		fout.close();
	}

	void AssetManager::LoadAssetRegistry()
	{
		if (!std::filesystem::exists(s_assetRegistryPath))
		{
			return;
		}

		std::ifstream file(s_assetRegistryPath);
		if (!file.is_open()) [[unlikely]]
		{
			LP_CORE_ERROR("Failed to open asset registry file: {0}!", s_assetRegistryPath.string().c_str());
			return;
		}

		std::stringstream strStream;
		strStream << file.rdbuf();
		file.close();

		YAML::Node root = YAML::Load(strStream.str());
		YAML::Node assets = root["Assets"];

		for (const auto entry : assets)
		{
			std::string path = entry["Path"].as<std::string>();
			AssetHandle handle = entry["Handle"].as<uint64_t>();

			m_assetRegistry.emplace(path, handle);
		}
	}
}