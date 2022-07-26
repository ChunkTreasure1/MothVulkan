#include "lppch.h"
#include "RenderPipelineRegistry.h"

#include "Lamp/Log/Log.h"

#include "Lamp/Asset/AssetManager.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipeline.h"

#include "Lamp/Utility/FileSystem.h"
#include "Lamp/Utility/StringUtility.h"

namespace Lamp
{
	void RenderPipelineRegistry::Initialize()
	{
		LoadAllPipelines();
	}

	void RenderPipelineRegistry::Shutdown()
	{
		s_registry.clear();
	}

	Ref<RenderPipeline> RenderPipelineRegistry::Get(const std::string& name)
	{
		std::string lowName = Utility::ToLower(name);
		auto it = s_registry.find(lowName);
		if (it == s_registry.end())
		{
			LP_CORE_ERROR("Unable to find shader {0}!", name.c_str());
			return nullptr;
		}

		return it->second;
	}

	std::map<std::string, Ref<RenderPipeline>> RenderPipelineRegistry::GetAllPipelines()
	{
		return s_registry;
	}

	void RenderPipelineRegistry::Register(const std::string& name, Ref<RenderPipeline> pipeline)
	{
		auto it = s_registry.find(name);
		if (it != s_registry.end())
		{
			LP_CORE_ERROR("Shader with that name has already been registered!");
			return;
		}

		std::string lowName = Utility::ToLower(name);
		s_registry[lowName] = pipeline;
	}

	void RenderPipelineRegistry::LoadAllPipelines()
	{
		auto shaderSearchFolder = FileSystem::GetRenderPipelinesPath();
		for (const auto& path : std::filesystem::recursive_directory_iterator(shaderSearchFolder))
		{
			AssetType type = AssetManager::Get().GetAssetTypeFromPath(path.path());
			if (type == AssetType::RenderPipeline)
			{
				Ref<RenderPipeline> pipeline = AssetManager::GetAsset<RenderPipeline>(path.path());
				Register(pipeline->GetSpecification().name, pipeline);
			}
		}
	}

}