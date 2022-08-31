#include "lppch.h"
#include "RenderPassRegistry.h"

#include "Lamp/Asset/AssetManager.h"
#include "Lamp/Asset/RenderPipelineAsset.h"

#include "Lamp/Rendering/RenderPass/RenderPass.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipelineRegistry.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipeline.h"
#include "Lamp/Rendering/Framebuffer.h"

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

	void RenderPassRegistry::SetupOverridesAndExclusives()
	{
		for (auto& [name, pass] : s_registry)
		{
			if (!pass->overridePipelineName.empty())
			{
				Ref<RenderPipelineAsset> overridePipeline = RenderPipelineRegistry::Get(pass->overridePipelineName);
				if (overridePipeline && overridePipeline->GetPipelineType() == PipelineType::Graphics)
				{
					pass->overridePipeline = overridePipeline->GetGraphicsPipeline();
				}
				else
				{
					LP_CORE_ERROR("Unable to find override pipeline {0} for render pass {1}!", pass->overridePipelineName.c_str(), pass->name.c_str());
				}
			}

			if (!pass->computePipelineName.empty())
			{
				Ref<RenderPipelineAsset> computePipeline = RenderPipelineRegistry::Get(pass->computePipelineName);
				if (computePipeline && computePipeline->GetPipelineType() == PipelineType::Compute)
				{
					pass->computePipeline = computePipeline->GetComputePipeline();
				}
			}

			if (!pass->exclusivePipelineName.empty())
			{
				Ref<RenderPipelineAsset> exclusivePipeline = RenderPipelineRegistry::Get(pass->exclusivePipelineName);
				if (exclusivePipeline && exclusivePipeline->GetPipelineType() == PipelineType::Graphics)
				{
					pass->exclusivePipelineHash = exclusivePipeline->GetGraphicsPipeline()->GetHash();
				}
				else
				{
					LP_CORE_ERROR("Unable to find exclusive pipeline {0} for render pass {1}!", pass->exclusivePipelineName.c_str(), pass->name.c_str());
				}
			}

			if (!pass->excludedPipelineNames.empty())
			{
				for (const auto& name : pass->excludedPipelineNames)
				{
					Ref<RenderPipelineAsset> excludedPipeline = RenderPipelineRegistry::Get(name);
					if (excludedPipeline && excludedPipeline->GetPipelineType() == PipelineType::Graphics)
					{
						pass->excludedPipelineHashes.emplace_back(excludedPipeline->GetGraphicsPipeline()->GetHash());
					}
					else
					{
						LP_CORE_ERROR("Unable to find excluded pipeline {0} for render pass {1}!", pass->exclusivePipelineName.c_str(), pass->name.c_str());
					}
				}
			}

		}
	}

	void RenderPassRegistry::SetupPassDependencies()
	{
		for (auto& [name, pass] : s_registry)
		{
			if (!pass->existingImages.empty())
			{
				FramebufferSpecification spec = pass->framebuffer->GetSpecification();
				for (const auto& image : pass->existingImages)
				{
					Ref<RenderPass> renderPass = Get(image.renderPass);
					if (renderPass)
					{
						Ref<Image2D> existingImage = image.depth ? renderPass->framebuffer->GetDepthAttachment() : renderPass->framebuffer->GetColorAttachment(image.attachmentIndex);
						if (existingImage && !image.depth)
						{
							spec.existingImages[image.targetIndex] = existingImage;
						}
						else if (image.depth)
						{
							spec.existingDepth = existingImage;
						}
						else
						{
							LP_CORE_ERROR("Render pass not found");
						}
					}

				}

				pass->framebuffer = Framebuffer::Create(spec);
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