#pragma once

#include "Lamp/Core/Base.h"

#include <unordered_map>

namespace Lamp
{
	class RenderPipeline;
	class RenderPipelineRegistry
	{
	public:
		static void Initialize();
		static void Shutdown();

		static Ref<RenderPipeline> Get(const std::string& name);
		static void Register(const std::string& name, Ref<RenderPipeline> pipeline);

	private:
		RenderPipelineRegistry() = delete;

		static void LoadAllPipelines();

		inline static std::unordered_map<std::string, Ref<RenderPipeline>> s_registry;
	};
}