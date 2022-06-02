#include "lppch.h"
#include "ShaderStorageBufferRegistry.h"

#include "Lamp/Log/Log.h"

namespace Lamp
{
	void ShaderStorageBufferRegistry::Initialize()
	{
	}

	void ShaderStorageBufferRegistry::Shutdowm()
	{
		s_registry.clear();
	}

	void ShaderStorageBufferRegistry::Register(uint32_t set, uint32_t binding, Ref<ShaderStorageBufferSet> shaderStorageBufferSet)
	{
		auto it = s_registry[set].find(binding);
		if (it != s_registry[set].end())
		{
			LP_CORE_ERROR("Shader storage buffer already registered to that binding!");
			return;
		}

		s_registry[set].emplace(binding, shaderStorageBufferSet);
	}

	Ref<ShaderStorageBufferSet> ShaderStorageBufferRegistry::Get(uint32_t set, uint32_t binding)
	{
		auto it = s_registry[set].find(binding);
		if (it == s_registry[set].end())
		{
			LP_CORE_ERROR("Shader storage buffer not registered to that binding!");
			return nullptr;
		}

		return it->second;
	}

}