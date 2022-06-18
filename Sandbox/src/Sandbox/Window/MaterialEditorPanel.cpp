#include "sbpch.h"
#include "MaterialEditorPanel.h"

#include <Lamp/Asset/MaterialRegistry.h>
#include <Lamp/Asset/Mesh/MultiMaterial.h>
#include <Lamp/Asset/Mesh/Material.h>
#include <Lamp/Asset/AssetManager.h>

#include <Lamp/Rendering/Texture/Texture2D.h>

#include <Lamp/Utility/UIUtility.h>

MaterialEditorPanel::MaterialEditorPanel()
	: EditorWindow("Material Editor", true)
{
	m_isOpen = true;
}

void MaterialEditorPanel::UpdateMainContent()
{
}

void MaterialEditorPanel::UpdateContent()
{
	ImGui::Begin("Materials");
	{
		if (ImGui::Button("Save selected material") && m_selectedMaterial)
		{
			Lamp::AssetManager::Get().SaveAsset(m_selectedMaterial);
		}

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
				std::string id = material->GetName() + "##" + std::to_string(index);
				if (ImGui::Selectable(id.c_str()))
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
			UI::PushId();
			if (UI::BeginProperties())
			{
				const auto textureDefinitions = m_selectedSubMaterial->GetTextureDefinitions();
				const auto textures = m_selectedSubMaterial->GetTextures();

				for (const auto [binding, name] : textureDefinitions)
				{
					auto it = textures.find(binding);
					if (it == textures.end())
					{
						LP_CORE_CRITICAL("Texture not found in material! Something has gone terribly wrong!");
						continue;
					}

					Ref<Lamp::Texture2D> texture = it->second;
					std::filesystem::path texturePath = texture->path;

					if (UI::Property<Lamp::Texture2D>(name, texture))
					{
						if (texture)
						{
							m_selectedSubMaterial->SetTexture(binding, texture);
						}
					}
				}

				UI::EndProperties();
			}
			UI::PopId();
		}
	}
	ImGui::End();
}
