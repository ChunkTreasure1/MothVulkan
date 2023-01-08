#include "lppch.h"
#include "MultiMaterial.h"

#include "Lamp/Rendering/Renderer.h"
#include "Lamp/Rendering/MaterialTable.h"

namespace Lamp
{
	MultiMaterial::MultiMaterial()
	{}

	void MultiMaterial::Construct()
	{
		for (const auto& [index, material] : m_materials)
		{
			Renderer::GetBindlessData().materialTable->AddMaterial(material);
		}
	}
}