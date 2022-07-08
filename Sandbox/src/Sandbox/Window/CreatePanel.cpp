#include "sbpch.h"
#include "CreatePanel.h"

#include <Lamp/Utility/UIUtility.h>
#include <Lamp/Components/Components.h>

CreatePanel::CreatePanel(std::vector<Lamp::Entity>& selectedEntites, Ref<Lamp::Scene>& scene)
	: EditorWindow("Create"), m_selectedEntites(selectedEntites), m_scene(scene)
{
	m_isOpen = true;
}

void CreatePanel::UpdateMainContent()
{
	UI::ScopedColor buttonColor(ImGuiCol_Button, { 0.313f, 0.313f, 0.313f, 1.f });
	UI::ScopedStyleFloat buttonRounding(ImGuiStyleVar_FrameRounding, 2.f);

	if (!m_brushListOpen)
	{
		const ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit;
		if (ImGui::BeginTable("createButtons", 2, tableFlags))
		{
			ImGui::TableSetupColumn("Column1", 0, ImGui::GetContentRegionAvail().x / 2.f);
			ImGui::TableSetupColumn("Column2", 0, ImGui::GetContentRegionAvail().x / 2.f);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			if (ImGui::Button("Entity", { ImGui::GetContentRegionAvail().x, m_buttonHeight }))
			{
				Lamp::Entity newEntity = m_scene->CreateEntity();

				{
					auto& comp = newEntity.AddComponent<Lamp::TransformComponent>();
					comp.position = { 0.f, 0.f, 0.f };
					comp.rotation = { 0.f, 0.f, 0.f };
					comp.scale = { 1.f, 1.f, 1.f };
				}

				auto& tag = newEntity.AddComponent<Lamp::TagComponent>();
				tag.tag = "New Entity";

				m_selectedEntites.clear();
				m_selectedEntites.emplace_back(newEntity);
			}

			ImGui::EndTable();
		}
	}
}