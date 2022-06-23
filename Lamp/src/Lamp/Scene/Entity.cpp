#include "lppch.h"
#include "Entity.h"

namespace Lamp
{
	Entity::Entity()
		: m_id(Wire::NullID)
	{
	}

	Entity::Entity(Wire::EntityId id, Scene* scene)
		: m_id(id), m_scene(scene)
	{
	}

	Entity::Entity(const Entity& entity)
	{
		*this = entity;
	}

	Entity::~Entity()
	{
	}

	std::unordered_map<WireGUID, std::vector<uint8_t>> Entity::GetComponents()
	{
		return m_scene->m_registry.GetComponents(m_id);
	}

	void Entity::SetComponents(const std::unordered_map<WireGUID, std::vector<uint8_t>>& components)
	{
		m_scene->m_registry.SetComponents(components, m_id);
	}

	Entity& Entity::operator=(const Entity& entity)
	{
		m_id = entity.m_id;
		m_scene = entity.m_scene;
		return *this;
	}
}