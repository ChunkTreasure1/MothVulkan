#include "lppch.h"
#include "AssetManager.h"

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
		LoadAssetRegistry();
	}

	void AssetManager::Shutdown()
	{
		SaveAssetRegistry();
	}

	AssetType AssetManager::GetAssetTypeFromPath(const std::filesystem::path& path)
	{
		return GetAssetTypeFromExtension(path.extension().string());
	}

	AssetType AssetManager::GetAssetTypeFromExtension(const std::string& extension)
	{
		std::string extension = Utility::ToLower(extension);
		if (s_assetExtensionsMap.find(extension) == s_assetExtensionsMap.end()) [[unlikely]]
		{
			return AssetType::None;
		}

		return s_assetExtensionsMap.at(extension);
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
			LP_CORE_ERROR("Failed to open asset registry file: {0}!", s_assetRegistryPath.string());
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
			AssetHandle handle = entry["Handle"].as<AssetHandle>();
			
			m_assetRegistry.emplace(path, handle);
		}
	}
}