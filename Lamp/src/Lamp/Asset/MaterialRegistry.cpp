#include "lppch.h"
#include "MaterialRegistry.h"

#include "Lamp/Log/Log.h"

#include "Lamp/Asset/AssetManager.h"
#include "Lamp/Asset/Mesh/MultiMaterial.h"

#include "Lamp/Utility/FileSystem.h"
#include "Lamp/Utility/StringUtility.h"

namespace Lamp
{
	void MaterialRegistry::Initialize()
	{
		LoadAllMaterials();
	}

	void MaterialRegistry::Shutdown()
	{
		s_registry.clear();
	}

	Ref<MultiMaterial> MaterialRegistry::Get(const std::string& name)
	{
		std::string lowName = Utility::ToLower(name);
		auto it = s_registry.find(lowName);
		if (it == s_registry.end())
		{
			LP_CORE_ERROR("Unable to find material {0}!", name.c_str());
			return nullptr;
		}

		return it->second;
	}

	void MaterialRegistry::Register(const std::string& name, Ref<MultiMaterial> shader)
	{
		auto it = s_registry.find(name);
		if (it != s_registry.end())
		{
			LP_CORE_ERROR("material with that name has already been registered!");
			return;
		}

		std::string lowName = Utility::ToLower(name);
		s_registry[lowName] = shader;
	}

	void MaterialRegistry::LoadAllMaterials()
	{
		auto shaderSearchFolder = FileSystem::GetAssetsPath();
		for (const auto& path : std::filesystem::recursive_directory_iterator(shaderSearchFolder))
		{
			AssetType type = AssetManager::Get().GetAssetTypeFromPath(path.path());
			if (type == AssetType::Material)
			{
				Ref<MultiMaterial> material = AssetManager::GetAsset<MultiMaterial>(path.path());
				if (material)
				{
					Register(material->GetName(), material);
				}
			}
		}
	}
}