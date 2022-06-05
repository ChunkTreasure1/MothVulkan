#pragma once

#include "Lamp/Core/Base.h"

#include <unordered_map>

namespace Lamp
{
	class ShaderStorageBufferSet;
	class ShaderStorageBufferRegistry
	{
	public:
		static void Initialize();
		static void Shutdowm();

		static void Register(uint32_t set, uint32_t binding, Ref<ShaderStorageBufferSet> uniformBufferSet);
		static Ref<ShaderStorageBufferSet> Get(uint32_t set, uint32_t binding);

	private:
		ShaderStorageBufferRegistry() = delete;

		inline static std::unordered_map<uint32_t, std::unordered_map<uint32_t, Ref<ShaderStorageBufferSet>>> s_registry; // set -> binding -> shaderStorageBufferSet
	};
}