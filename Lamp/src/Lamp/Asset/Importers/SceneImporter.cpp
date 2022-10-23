#include "lppch.h"
#include "SceneImporter.h"

#include "Lamp/Scene/Scene.h"

#include "Lamp/Utility/YAMLSerializationHelpers.h"
#include "Lamp/Utility/SerializationMacros.h"

#include "Lamp/Log/Log.h"

#include <Wire/Serialization.h>
#include <yaml-cpp/yaml.h>

namespace Lamp
{
	bool SceneImporter::Load(const std::filesystem::path& path, Ref<Asset>& asset) const
	{
		Ref<Scene> scene = reinterpret_pointer_cast<Scene>(asset);

		if (!std::filesystem::exists(path)) [[unlikely]]
		{
			LP_CORE_ERROR("File {0} not found!", path.string().c_str());
			asset->SetFlag(AssetFlag::Missing, true);
			return false;
		}

		std::ifstream file(path);
		if (!file.is_open()) [[unlikely]]
		{
			LP_CORE_ERROR("Failed to open file {0}!", path.string().c_str());
			asset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		std::filesystem::path scenePath = path;
		std::filesystem::path folderPath = scenePath.parent_path();
		std::filesystem::path entitiesFolderPath = folderPath / "Entities";

		std::stringstream sstream;
		sstream << file.rdbuf();
		file.close();

		YAML::Node root = YAML::Load(sstream.str());
		YAML::Node sceneNode = root["Scene"];

		LP_DESERIALIZE_PROPERTY(name, scene->m_name, sceneNode, std::string("New Scene"));

		if (std::filesystem::exists(entitiesFolderPath))
		{
			for (const auto& file : std::filesystem::directory_iterator(entitiesFolderPath))
			{
				Wire::Serializer::DeserializeEntityToRegistry(file.path(), scene->m_registry);
			}
		}

		return true;
	}

	void SceneImporter::Save(const Ref<Asset>& asset) const
	{
		const Ref<Scene> scene = std::reinterpret_pointer_cast<Scene>(asset);

		std::filesystem::path folderPath = asset->path;
		std::filesystem::path scenePath = folderPath / (asset->path.stem().string() + ".lpscene");
		std::filesystem::path entitiesFolderPath = folderPath / "Entities";

		if (!std::filesystem::exists(folderPath))
		{
			std::filesystem::create_directories(folderPath);
		}

		if (!std::filesystem::exists(entitiesFolderPath))
		{
			std::filesystem::create_directories(entitiesFolderPath);
		}

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value;
		{
			out << YAML::BeginMap;
			LP_SERIALIZE_PROPERTY(name, scene->GetName(), out);
			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		std::ofstream file;
		file.open(scenePath);
		file << out.c_str();
		file.close();

		/////Entities//////
		for (const auto entity : scene->GetRegistry().GetAllEntities())
		{
			Wire::Serializer::SerializeEntityToFile(entity, scene->GetRegistry(), entitiesFolderPath);
		}
	}
}