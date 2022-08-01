#include "sbpch.h"
#include "SceneViewPanel.h"

#include <Lamp/Utility/UIUtility.h>
#include <Lamp/Components/Components.h>

SceneViewPanel::SceneViewPanel(std::vector<Lamp::Entity>& selectedEntities, Ref<Lamp::Scene>& scene)
	: EditorWindow("Scene View"), m_selectedEntities(selectedEntities), m_scene(scene)
{
	m_isOpen = true;
}

void SceneViewPanel::UpdateMainContent()
{
	const float imageSize = 20.f;
	const float imagePadding = 5.f;

	UI::ScopedColor button(ImGuiCol_Button, { 0.f, 0.f, 0.f, 0.f });
	UI::ScopedColor hovered(ImGuiCol_ButtonHovered, { 0.3f, 0.305f, 0.31f, 0.5f });
	UI::ScopedColor active(ImGuiCol_ButtonActive, { 0.5f, 0.505f, 0.51f, 0.5f });
	UI::ScopedColor tableRow(ImGuiCol_TableRowBg, { 0.18f, 0.18f, 0.18f, 1.f });

	if (ImGui::BeginTable("entitiesTable", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable))
	{
		ImGui::TableSetupColumn("Modifiers", ImGuiTableColumnFlags_WidthFixed, imageSize * 2.f + imagePadding);
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Type");
		ImGui::TableHeadersRow();

		for (auto entity : m_scene->GetRegistry().GetAllEntities())
		{
			ImGui::PushID(entity);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			// Modifiers
			{
				ImGui::TableNextColumn();
			}

			// Name
			{
				std::string name = "Entity";
				if (m_scene->GetRegistry().HasComponent<Lamp::TagComponent>(entity))
				{
					name = m_scene->GetRegistry().GetComponent<Lamp::TagComponent>(entity).tag;
				}

				name += "##" + std::to_string(entity);
				if (ImGui::Selectable(name.c_str()))
				{
				}

				ImGui::TableNextColumn();
			}

			// Type
			{
				ImGui::TextDisabled("Entity");
			}

			ImGui::PopID();
		}

		ImGui::EndTable();
	}
}
