#pragma once

#include "Lamp/Core/Base.h"
#include "Lamp/Asset/Asset.h"

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

	class Scene : public Asset
	{
	public:
		Scene() = default;
		Scene(const std::string & name);
		Scene(const Scene & scene);

		void OnRuntimeStart();
		void OnRuntimeEnd();
		void Update(float deltaTime);

		Entity CreateEntity();
		void RemoveEntity(const Entity & entity);

		static AssetType GetStaticType() { return AssetType::Scene; }
		AssetType GetType() override { return GetStaticType(); }

		inline Wire::Registry& GetRegistry() { return m_registry; }
		inline const std::string& GetName() const { return m_name; }

	private:
		friend class Entity;
		friend class SceneImporter;

		SceneEnvironment m_environment;

		std::string m_name = "New Scene";
		Wire::Registry m_registry;
	};
}