#include "sbpch.h"
#include "SceneViewPanel.h"

#include "Sandbox/Window/EditorIconLibrary.h"

#include <Lamp/Input/Input.h>
#include <Lamp/Input/KeyCodes.h>

#include <Lamp/Utility/UIUtility.h>
#include <Lamp/Utility/StringUtility.h>

#include <Lamp/Components/Components.h>

SceneViewPanel::SceneViewPanel(std::vector<Wire::EntityId>& selectedEntities, Ref<Lamp::Scene>& scene)
	: EditorWindow("Scene View"), m_selectedEntities(selectedEntities), m_scene(scene)
{
	m_isOpen = true;
}

void SceneViewPanel::UpdateMainContent()
{
	const float imageSize = 20.f;
	const float imagePadding = 5.f;

	RenderSearchBar();

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

		const std::vector<Wire::EntityId> usedEntities = m_hasSearchQuery ? m_entities : m_scene->GetRegistry().GetAllEntities();

		for (auto entity : usedEntities)
		{
			ImGui::PushID(entity);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			// Modifiers
			{
				if (m_scene->GetRegistry().HasComponent<Lamp::TransformComponent>(entity))
				{
					auto& transformComponent = m_scene->GetRegistry().GetComponent<Lamp::TransformComponent>(entity);

					Ref<Lamp::Texture2D> visibleIcon = transformComponent.visible ? EditorIconLibrary::GetIcon(EditorIcon::Visible) : EditorIconLibrary::GetIcon(EditorIcon::Hidden);
					std::string visibleId = "##visible" + std::to_string(entity);
					if (UI::ImageButton(visibleId, UI::GetTextureID(visibleIcon), { imageSize, imageSize }, { 0.f, 0.f }, { 1.f, 1.f }, 0))
					{
						transformComponent.visible = !transformComponent.visible;
					}

					ImGui::SameLine();

					Ref<Lamp::Texture2D> lockedIcon = transformComponent.locked ? EditorIconLibrary::GetIcon(EditorIcon::Locked) : EditorIconLibrary::GetIcon(EditorIcon::Unlocked);
					std::string lockedId = "##locked" + std::to_string(entity);
					if (UI::ImageButton(lockedId, UI::GetTextureID(lockedIcon), { imageSize, imageSize }, { 0.f, 0.f }, { 1.f, 1.f }, 0))
					{
						transformComponent.locked = !transformComponent.locked;
					}
				}

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

				auto it = std::find(m_selectedEntities.begin(), m_selectedEntities.end(), entity);
				bool selected = it != m_selectedEntities.end() ? true : false;

				if (ImGui::Selectable(name.c_str(), selected))
				{
					if (Lamp::Input::IsKeyDown(LP_KEY_LEFT_CONTROL))
					{
						if (it != m_selectedEntities.end())
						{
							m_selectedEntities.erase(it);
						}
						else
						{
							m_selectedEntities.emplace_back(entity);
						}
					}
					else
					{
						if (it != m_selectedEntities.end() && m_selectedEntities.size() == 1)
						{
							m_selectedEntities.erase(it);
						}
						else
						{
							m_selectedEntities.clear();
							m_selectedEntities.emplace_back(entity);
						}
					}
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

void SceneViewPanel::RenderSearchBar()
{
	UI::ScopedColor childColor{ ImGuiCol_ChildBg, { 0.2f, 0.2f, 0.2f, 1.f } };

	constexpr float barHeight = 32.f;
	constexpr float searchBarSize = 22.f;

	ImGui::BeginChild("##searchBar", { 0.f, barHeight });
	{
		UI::ShiftCursor(5.f, 4.f);
		ImGui::Image(UI::GetTextureID(EditorIconLibrary::GetIcon(EditorIcon::Search)), { searchBarSize, searchBarSize });

		ImGui::SameLine();

		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x);

		UI::PushId();
		if (UI::InputText("", m_searchQuery, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			if (!m_searchQuery.empty())
			{
				m_hasSearchQuery = true;
				Search(m_searchQuery);
			}
			else
			{
				m_hasSearchQuery = false;
			}
		}
		UI::PopId();

		ImGui::PopItemWidth();
	}
	ImGui::EndChild();
}

void SceneViewPanel::Search(const std::string query)
{
	const std::string lowerQuery = Utility::ToLower(query);

	m_entities.clear();

	for (auto entity : m_scene->GetRegistry().GetAllEntities())
	{
		if (m_scene->GetRegistry().HasComponent<Lamp::TagComponent>(entity))
		{
			auto tagComp = m_scene->GetRegistry().GetComponent<Lamp::TagComponent>(entity);
			const std::string lowerName = Utility::ToLower(tagComp.tag);

			if (lowerName.find(lowerQuery) != std::string::npos)
			{
				m_entities.emplace_back(entity);
			}
		}
	}
}
