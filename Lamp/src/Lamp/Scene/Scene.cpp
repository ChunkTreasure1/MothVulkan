#include "lppch.h"
#include "Scene.h"

#include "Entity.h"

#include "Lamp/Components/Components.h"

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
	}

	void Scene::OnRuntimeEnd()
	{
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