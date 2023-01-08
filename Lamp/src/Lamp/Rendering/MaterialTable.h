#pragma once

#include "Lamp/Core/Base.h"

namespace Lamp
{
	class Material;
	class MaterialTable
	{
	public:
		MaterialTable() = default;

		const int32_t GetMaterialId(const Ref<Material> material);
		const uint32_t AddMaterial(const Ref<Material> material);

		inline const auto& GetTable() const { return m_materialToIdMap; };

		static Ref<MaterialTable> Create();

	private:
		std::map<Ref<Material>, uint32_t> m_materialToIdMap;
	};
}