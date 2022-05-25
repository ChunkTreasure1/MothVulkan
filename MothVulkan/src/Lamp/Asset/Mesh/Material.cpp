#include "lppch.h"
#include "Material.h"

namespace Lamp
{
	Material::Material(const std::string& name, uint32_t index)
		: m_name(name), m_index(index)
	{
	}

	Ref<Material> Material::Create(const std::string& name, uint32_t index)
	{
		return CreateRef<Material>(name, index);
	}
}
