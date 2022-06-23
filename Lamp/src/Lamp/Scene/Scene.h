#pragma once

#include "Lamp/Core/Base.h"

#include <Wire/Registry.h>

namespace Lamp
{
	class Entity;
	class TextureCube;

	struct SceneEnvironment
	{
		Ref<TextureCube> irradianceMap;
		Ref<TextureCube> radianceMap;
	};

	class Scene
	{
	public:
		Scene() = default;
		Scene(const std::string & name);
		Scene(const Scene & scene);

		void OnRuntimeStart();
		void OnRuntimeEnd();

		Entity CreateEntity();
		void RemoveEntity(const Entity & entity);

		inline Wire::Registry& GetRegistry() { return m_registry; }

	private:
		friend class Entity;

		SceneEnvironment m_environment;

		std::string m_name;
		Wire::Registry m_registry;
	};
}