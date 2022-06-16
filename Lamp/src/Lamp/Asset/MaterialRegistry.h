#pragma once

#include "Lamp/Core/Base.h"

#include <unordered_map>

namespace Lamp
{
	class MultiMaterial;
	class MaterialRegistry
	{
	public:
		static void Initialize();
		static void Shutdown();

		static Ref<MultiMaterial> Get(const std::string& name);
		static void Register(const std::string& name, Ref<MultiMaterial> shader);

		inline static const std::unordered_map<std::string, Ref<MultiMaterial>>& GetMaterials() { return s_registry; }

	private:
		MaterialRegistry() = delete;

		static void LoadAllMaterials();

		inline static std::unordered_map<std::string, Ref<MultiMaterial>> s_registry;
	};
}