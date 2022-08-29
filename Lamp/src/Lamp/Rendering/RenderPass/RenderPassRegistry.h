#pragma once

#include "Lamp/Core/Base.h"

#include <unordered_map>

namespace Lamp
{
	class RenderPass;
	class RenderPassRegistry
	{
	public:
		static void Initialize();
		static void Shutdown();

		static void SetupOverridesAndExclusives();

		static Ref<RenderPass> Get(const std::string& name);
		static void Register(const std::string& name, Ref<RenderPass> pipeline);

		static std::map<std::string, Ref<RenderPass>> GetAllPasses();

	private:
		RenderPassRegistry() = delete;

		static void LoadAllPasses();

		inline static std::map<std::string, Ref<RenderPass>> s_registry;
	};
}