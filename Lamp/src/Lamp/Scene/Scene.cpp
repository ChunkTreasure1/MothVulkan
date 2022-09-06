#include "lppch.h"
#include "Scene.h"

#include "Entity.h"

#include "Lamp/Components/Components.h"
#include "Lamp/Scripting/ScriptEngine.h"

namespace Lamp
{
	Scene::Scene(const std::string& name)
		: m_name(name)
	{
	}

	Scene::Scene(const Scene& scene)
	{
		m_name = scene.m_name;
	}

	void Scene::OnRuntimeStart()
	{
		m_registry.ForEach<ScriptComponent>([](Wire::EntityId id, const ScriptComponent& scriptComp)
			{
				//auto script = ScriptEngine::GetScript(id, scriptComp.scripts[0]);
				//script->OnStart();
			});
	}

	void Scene::OnRuntimeEnd()
	{
		m_registry.ForEach<ScriptComponent>([](Wire::EntityId id, const ScriptComponent& scriptComp)
			{
				//auto script = ScriptEngine::GetScript(id, scriptComp.scripts[0]);
				//script->OnStop();
			});
	}

	void Scene::Update(float deltaTime)
	{
		m_registry.ForEach<ScriptComponent>([deltaTime](Wire::EntityId id, const ScriptComponent& scriptComp)
			{
				//auto script = ScriptEngine::GetScript(id, scriptComp.scripts[0]);
				//script->OnUpdate(deltaTime);
			});
	}

	Entity Scene::CreateEntity()
	{
		Wire::EntityId id = m_registry.CreateEntity();

		Entity newEntity = Entity(id, this);
		auto& transform = newEntity.AddComponent<TransformComponent>();
		transform.position = { 0.f, 0.f, 0.f };
		transform.rotation = { 0.f, 0.f, 0.f };
		transform.scale = { 1.f, 1.f, 1.f };

		auto& tag = newEntity.AddComponent<TagComponent>();
		tag.tag = "New Entity";

		return newEntity;
	}

	void Scene::RemoveEntity(const Entity& entity)
	{
		m_registry.RemoveEntity(entity.GetId());
	}
}