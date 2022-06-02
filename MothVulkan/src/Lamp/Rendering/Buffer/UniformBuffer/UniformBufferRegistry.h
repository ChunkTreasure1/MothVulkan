#pragma once

#include "Lamp/Core/Base.h"

#include <unordered_map>

namespace Lamp
{
	class UniformBufferSet;
	class UniformBufferRegistry
	{
	public:
		static void Initialize();
		static void Shutdowm();

		static void Register(uint32_t set, uint32_t binding, Ref<UniformBufferSet> uniformBufferSet);
		static Ref<UniformBufferSet> Get(uint32_t set, uint32_t binding);

	private:
		UniformBufferRegistry() = delete;
	
		inline static std::unordered_map<uint32_t, std::unordered_map<uint32_t, Ref<UniformBufferSet>>> s_registry; // set -> binding -> uniformBufferSet
	};
}