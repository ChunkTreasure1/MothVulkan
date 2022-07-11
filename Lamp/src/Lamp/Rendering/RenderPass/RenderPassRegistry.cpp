#include "lppch.h"
#include "RenderPassRegistry.h"

#include "Lamp/Asset/AssetManager.h"
#include "Lamp/Rendering/RenderPass/RenderPass.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipelineRegistry.h"

#include "Lamp/Utility/StringUtility.h"
#include "Lamp/Utility/FileSystem.h"

namespace Lamp
{
	void RenderPassRegistry::Initialize()
	{
		LoadAllPasses();
	}

	void RenderPassRegistry::Shutdown()
	{
		s_registry.clear();
	}

	void RenderPassRegistry::SetupOverrides()
	{
		for (auto& [name, pass] : s_registry)
		{
			if (!pass->overridePipelineName.empty())
			{
				Ref<RenderPipeline> overridePipeline = RenderPipelineRegistry::Get(pass->overridePipelineName);
				if (overridePipeline)
				{
					pass->overridePipeline = overridePipeline;
				}
				else
				{
					LP_CORE_ERROR("Unable to find override pipeline {0} for render pass {1}!", pass->overridePipelineName.c_str(), pass->name.c_str());
				}
			}
		}
	}

	Ref<RenderPass> RenderPassRegistry::Get(const std::string& name)
	{
		std::string lowName = Utility::ToLower(name);
		auto it = s_registry.find(lowName);
		if (it == s_registry.end())
		{
			LP_CORE_ERROR("Unable to find render pass {0}!", name.c_str());
			return nullptr;
		}

		return it->second;
	}

	void RenderPassRegistry::Register(const std::string& name, Ref<RenderPass> pipeline)
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

	std::map<std::string, Ref<RenderPass>> RenderPassRegistry::GetAllPasses()
	{
		return s_registry;
	}

	void RenderPassRegistry::LoadAllPasses()
	{
		auto shaderSearchFolder = FileSystem::GetRenderPassesPath();
		for (const auto& path : std::filesystem::recursive_directory_iterator(shaderSearchFolder))
		{
			AssetType type = AssetManager::Get().GetAssetTypeFromPath(path.path());
			if (type == AssetType::RenderPass)
			{
				Ref<RenderPass> pass = AssetManager::GetAsset<RenderPass>(path.path());
				Register(pass->name, pass);
			}
		}
	}

}