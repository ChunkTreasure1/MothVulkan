#pragma once

#include "Lamp/Core/Base.h"

#include <unordered_map>

namespace Lamp
{
	class RenderPipelineAsset;
	class RenderPipelineRegistry
	{
	public:
		static void Initialize();
		static void Shutdown();

		static Ref<RenderPipelineAsset> Get(const std::string& name);
		static std::map<std::string, Ref<RenderPipelineAsset>> GetAllPipelines();
		static void Register(const std::string& name, Ref<RenderPipelineAsset> pipeline);

	private:
		RenderPipelineRegistry() = delete;

		static void LoadAllPipelines();

		inline static std::map<std::string, Ref<RenderPipelineAsset>> s_registry;
	};
}