#include "sbpch.h"
#include "MaterialEditorPanel.h"

#include "Lamp/Asset/MaterialRegistry.h"
#include "Lamp/Asset/Mesh/MultiMaterial.h"
#include "Lamp/Asset/Mesh/Material.h"

MaterialEditorPanel::MaterialEditorPanel()
	: EditorWindow("Material Editor", true)
{
	m_isOpen = true;
}

void MaterialEditorPanel::UpdateMainContent()
{}

void MaterialEditorPanel::UpdateContent()
{
	ImGui::Begin("Materials");
	{
		const auto& materials = Lamp::MaterialRegistry::GetMaterials();
		for (auto& [name, material] : materials)
		{
			if (ImGui::Selectable(name.c_str()))
			{
				m_selectedMaterial = material;
			}
		}
	}
	ImGui::End();

	ImGui::Begin("Sub materials");
	{
		if (m_selectedMaterial)
		{
			const auto& subMaterials = m_selectedMaterial->GetMaterials();
			for (auto& [index, material] : subMaterials)
			{
				if (ImGui::Selectable(std::to_string(index).c_str()))
				{
					m_selectedSubMaterial = material;
				}
			}
		}
	}
	ImGui::End();

	ImGui::Begin("Properties##materialEditor");
	{
		if (m_selectedSubMaterial)
		{
			for (const auto& [index, texture] : m_selectedSubMaterial->GetTextures())
			{
				ImGui::Text("%d", index);
			}
		}
	}
	ImGui::End();
}
