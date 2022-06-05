#include "lppch.h"
#include "UniformBufferRegistry.h"

#include "Lamp/Log/Log.h"

namespace Lamp
{
	void UniformBufferRegistry::Initialize()
	{
	}
	
	void UniformBufferRegistry::Shutdowm()
	{
		for (auto& set : s_registry)
		{
			set.second.clear();
		}

		s_registry.clear();
	}

	void UniformBufferRegistry::Register(uint32_t set, uint32_t binding, Ref<UniformBufferSet> uniformBufferSet)
	{
		auto it = s_registry[set].find(binding);
		if (it != s_registry[set].end())
		{
			LP_CORE_ERROR("Uniform buffer already registered to that binding!");
			return;
		}

		s_registry[set].emplace(binding, uniformBufferSet);
	}
	
	Ref<UniformBufferSet> UniformBufferRegistry::Get(uint32_t set, uint32_t binding)
	{
		auto it = s_registry[set].find(binding);
		if (it == s_registry[set].end())
		{
			LP_CORE_ERROR("Uniform buffer not registered to that binding!");
			return nullptr;
		}
		
		return it->second;
	}
}