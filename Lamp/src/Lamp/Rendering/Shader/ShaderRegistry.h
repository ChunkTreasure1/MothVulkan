#pragma once

#include "Lamp/Core/Base.h"

#include <unordered_map>

namespace Lamp
{
	class Shader;
	class ShaderRegistry
	{
	public:
		static void Initialize();
		static void Shutdown();

		static Ref<Shader> Get(const std::string& name);
		static void Register(const std::string& name, Ref<Shader> shader);

		static std::map<std::string, Ref<Shader>> GetAllShaders();

	private:
		ShaderRegistry() = delete;

		static void LoadAllShaders();

		inline static std::map<std::string, Ref<Shader>> s_registry;
	};
}