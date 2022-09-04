#include "lppch.h"
#include "AssetImporter.h"

#include "Lamp/Log/Log.h"
#include "Lamp/Asset/Mesh/MultiMaterial.h"
#include "Lamp/Asset/Mesh/Material.h"
#include "Lamp/Asset/AssetManager.h"

#include "Lamp/Rendering/RenderPipeline/RenderPipelineRegistry.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipeline.h"

#include "Lamp/Rendering/Texture/Texture2D.h"
#include "Lamp/Rendering/Shader/Shader.h"
#include "Lamp/Rendering/Renderer.h"

#include "Lamp/Utility/SerializationMacros.h"

#include <yaml-cpp/yaml.h>

namespace Lamp
{
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

		if (!root["paths"]) [[unlikely]]
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

		YAML::Node inputTexturesNode = root["inputTextures"];
		std::unordered_map<uint32_t, std::string> inputTextures; // binding -> name
		for (const auto input : inputTexturesNode)
		{
			uint32_t binding;
			LP_DESERIALIZE_PROPERTY(binding, binding, input, 0);

			std::string name;
			LP_DESERIALIZE_PROPERTY(name, name, input, std::string("Null"));

			inputTextures.emplace(binding, name);
		}

		Ref<Shader> shader = Shader::Create(name, paths, false);

		// Make sure all textures defined in definition actually exist
		{
			const auto& imageInfos = shader->GetResources().imageInfos;
			auto& shaderInputDefinitions = const_cast<std::unordered_map<uint32_t, std::string>&>(shader->GetResources().shaderTextureDefinitions);

			auto setIt = imageInfos.find((uint32_t)DescriptorSetType::PerMaterial);
			if (setIt != imageInfos.end())
			{
				for (const auto& [binding, name] : inputTextures)
				{
					auto inputIt = setIt->second.find(binding);
					if (inputIt == setIt->second.end())
					{
						LP_CORE_ERROR("Shader {0} does not have a texture input with binding {1}, but this is defined in definition!", path.string().c_str(), binding);
					}
					else
					{
						shaderInputDefinitions.emplace(binding, name);
					}
				}
			}
		}

		if (!shader) [[unlikely]]
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

	bool MultiMaterialImporter::Load(const std::filesystem::path& path, Ref<Asset>& asset) const
	{
		asset = CreateRef<MultiMaterial>();
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
		YAML::Node multiMaterialNode = root["MultiMaterial"];

		std::string nameString;
		LP_DESERIALIZE_PROPERTY(name, nameString, multiMaterialNode, std::string("Null"));

		std::unordered_map<uint32_t, Ref<Material>> materials;

		YAML::Node materialsNode = multiMaterialNode["materials"];
		for (const auto& materialNode : materialsNode)
		{
			std::string materialNameString;
			LP_DESERIALIZE_PROPERTY(material, materialNameString, materialNode, std::string("Null"));

			uint32_t materialIndex;
			LP_DESERIALIZE_PROPERTY(index, materialIndex, materialNode, 0);

			std::string renderPipelineString;
			LP_DESERIALIZE_PROPERTY(renderPipeline, renderPipelineString, materialNode, std::string("Null"));

			std::unordered_map<uint32_t, Ref<Texture2D>> textures;

			YAML::Node texturesNode = materialNode["textures"];
			for (const auto& textureNode : texturesNode)
			{
				uint32_t textureBinding;
				LP_DESERIALIZE_PROPERTY(binding, textureBinding, textureNode, 0);

				AssetHandle textureHandle;
				LP_DESERIALIZE_PROPERTY(handle, textureHandle, textureNode, uint64_t(0));

				Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(textureHandle);
				if (textureHandle == Asset::Null() || !texture->IsValid())
				{
					texture = Renderer::GetDefaultData().whiteTexture;
				}

				textures.emplace(textureBinding, texture);
			}

			Ref<RenderPipeline> renderPipeline = RenderPipelineRegistry::Get(renderPipelineString);
			if (!renderPipeline || !renderPipeline->IsValid())
			{
				renderPipeline = Renderer::GetDefaultData().defaultPipeline;
				LP_CORE_ERROR("Render pipeline {0} not found or invalid! Fallingback to default!", renderPipelineString);
			}

			Ref<Material> material = Material::Create(materialNameString, materialIndex, renderPipeline);
			for (const auto& [binding, texture] : textures)
			{
				if (renderPipeline->GetSpecification().shader->GetResources().shaderTextureDefinitions.find(binding) != renderPipeline->GetSpecification().shader->GetResources().shaderTextureDefinitions.end())
				{
					material->SetTexture(binding, texture);
				}
			}

			materials.emplace(materialIndex, material);
		}

		Ref<MultiMaterial> multiMaterial = std::reinterpret_pointer_cast<MultiMaterial>(asset);
		multiMaterial->m_name = nameString;
		multiMaterial->m_materials = materials;
		multiMaterial->path = path;

		return true;
	}

	void MultiMaterialImporter::Save(const Ref<Asset>& asset) const
	{
		Ref<MultiMaterial> material = std::reinterpret_pointer_cast<MultiMaterial>(asset);

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "MultiMaterial" << YAML::Value;
		{
			out << YAML::BeginMap;
			LP_SERIALIZE_PROPERTY(name, material->m_name, out);

			{
				out << YAML::Key << "materials" << YAML::BeginSeq;
				for (const auto& [index, material] : material->m_materials)
				{
					out << YAML::BeginMap;
					LP_SERIALIZE_PROPERTY(material, material->GetName(), out);
					LP_SERIALIZE_PROPERTY(index, index, out);
					LP_SERIALIZE_PROPERTY(renderPipeline, material->m_renderPipeline->GetSpecification().name, out);

					out << YAML::Key << "textures" << YAML::BeginSeq;
					for (const auto& [binding, texture] : material->m_textures)
					{
						out << YAML::BeginMap;
						LP_SERIALIZE_PROPERTY(binding, binding, out);
						LP_SERIALIZE_PROPERTY(handle, texture->handle, out);
						out << YAML::EndMap;
					}
					out << YAML::EndSeq;
					out << YAML::EndMap;
				}
				out << YAML::EndSeq;
			}
			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		std::ofstream fout(asset->path);
		fout << out.c_str();
		fout.close();
	}
}