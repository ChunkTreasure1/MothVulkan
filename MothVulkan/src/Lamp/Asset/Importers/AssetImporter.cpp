#include "lppch.h"
#include "AssetImporter.h"

#include "Lamp/Log/Log.h"
#include "Lamp/Asset/Importers/TextureImporter.h"

#include "Lamp/Rendering/Texture/Texture2D.h"
#include "Lamp/Rendering/Shader/Shader.h"

#include <yaml-cpp/yaml.h>

namespace Lamp
{
	bool TextureSourceImporter::Load(const std::filesystem::path& path, Ref<Asset>& asset) const
	{
		asset = CreateRef<Texture2D>();

		if (!std::filesystem::exists(path)) [[unlikely]]
		{
			LP_CORE_ERROR("File {0} not found!", path.string().c_str());
			asset->SetFlag(AssetFlag::Missing, true);
			return false;
		}
		auto mesh = TextureImporter::ImportTexture(path);

		if (!mesh) [[unlikely]]
		{
			asset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		asset = mesh;
		asset->path = path;
		return true;
	}

	void TextureSourceImporter::Save(const Ref<Asset>& asset) const
	{
	}

	bool ShaderImporter::Load(const std::filesystem::path& path, Ref<Asset>& asset) const
	{
		asset = CreateRef<Shader>();
		if (!std::filesystem::exists(path)) [[unlikely]]
		{
			LP_CORE_ERROR("File {0} not found!", path.string().c_str());
			asset->SetFlag(AssetFlag::Missing, true);
			return false;
		}

		std::ifstream file(path);
		if (!file.is_open()) [[unlikely]]
		{
			LP_CORE_ERROR("Failed to open file: {0}!", path.string().c_str());
			asset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		std::stringstream sstream;
		sstream << file.rdbuf();
		file.close();

		YAML::Node root = YAML::Load(sstream.str());
		std::string name = root["name"] ? root["name"].as<std::string>() : "Unnamed";
		
		if (!root["paths"])
		{
			LP_CORE_ERROR("No shaders defined in shader definition {0}!", path.string().c_str());
			asset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		YAML::Node pathsNode = root["paths"];
		std::vector<std::filesystem::path> paths;
		
		for (const auto path : pathsNode)
		{
			paths.emplace_back(path.as<std::string>());
		}

		Ref<Shader> shader = Shader::Create(name, paths, false);
		if (!shader)
		{
			LP_CORE_ERROR("Failed to create shader {0}!", name.c_str());
			asset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		asset = shader;
		asset->path = path;
		
		return true;
	}

	void ShaderImporter::Save(const Ref<Asset>& asset) const
	{

	}
}