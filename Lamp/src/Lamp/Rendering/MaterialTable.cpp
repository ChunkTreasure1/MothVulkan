#include "lppch.h"
#include "MaterialTable.h"

namespace Lamp
{
	const int32_t MaterialTable::GetMaterialId(const Ref<Material> material)
	{
		if (!m_materialToIdMap.contains(material))
		{
			return -1;
		}

		return (int32_t)m_materialToIdMap.at(material);
	}

	const uint32_t MaterialTable::AddMaterial(const Ref<Material> material)
	{
		if (m_materialToIdMap.contains(material))
		{
			return m_materialToIdMap.at(material);
		}

		const uint32_t id = (uint32_t)m_materialToIdMap.size();
		m_materialToIdMap.emplace(material, id);

		return id;
	}
	
	Ref<MaterialTable> MaterialTable::Create()
	{
		return CreateRef<MaterialTable>();
	}
}