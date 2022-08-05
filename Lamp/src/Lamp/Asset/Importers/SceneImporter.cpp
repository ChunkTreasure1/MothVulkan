#include "lppch.h"
#include "SceneImporter.h"

#include "Lamp/Scene/Scene.h"

#include "Lamp/Utility/YAMLSerializationHelpers.h"
#include "Lamp/Utility/SerializationMacros.h"

#include <Wire/Serialization.h>
#include <yaml-cpp/yaml.h>

namespace Lamp
{
	bool SceneImporter::Load(const std::filesystem::path& path, Ref<Asset>& asset) const
	{
		return false;
	}

	void SceneImporter::Save(const Ref<Asset>& asset) const
	{
		const Ref<Scene> scene = std::reinterpret_pointer_cast<Scene>(asset);

		std::filesystem::path folderPath = asset->path.parent_path() / asset->path.stem();
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