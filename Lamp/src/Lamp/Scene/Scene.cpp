#include "lppch.h"
#include "Scene.h"

#include "Entity.h"

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
		return Entity(id, this);
	}

	void Scene::RemoveEntity(const Entity& entity)
	{
		m_registry.RemoveEntity(entity.GetId());
	}
}