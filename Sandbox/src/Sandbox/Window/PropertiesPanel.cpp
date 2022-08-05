#include "sbpch.h"
#include "PropertiesPanel.h"

#include <Lamp/Components/Components.h>
#include <Lamp/Utility/UIUtility.h>

#include <Wire/Serialization.h>

PropertiesPanel::PropertiesPanel(std::vector<Wire::EntityId>& selectedEntites, Ref<Lamp::Scene>& currentScene)
	: EditorWindow("Properties"), m_selectedEntites(selectedEntites), m_currentScene(currentScene)
{
	m_isOpen = true;
}

void PropertiesPanel::UpdateMainContent()
{
	if (m_selectedEntites.empty())
	{
		return;
	}

	const bool singleSelected = !(m_selectedEntites.size() > 1);
	auto& registry = m_currentScene->GetRegistry();
	const auto firstEntity = m_selectedEntites.front();

	if (singleSelected)
	{
		if (registry.HasComponent<Lamp::TagComponent>(firstEntity))
		{
			auto& tag = registry.GetComponent<Lamp::TagComponent>(firstEntity);
			UI::InputText("Name", tag.tag);
		}
	}
	else
	{
		static std::string inputText = "...";

		std::string firstName;
		bool sameName = true;

		if (registry.HasComponent<Lamp::TagComponent>(firstEntity))
		{
			firstName = registry.GetComponent<Lamp::TagComponent>(firstEntity).tag;
		}

		for (auto& entity : m_selectedEntites)
		{
			if (registry.HasComponent<Lamp::TagComponent>(entity))
			{
				auto& tag = registry.GetComponent<Lamp::TagComponent>(entity);
				if (tag.tag != firstName)
				{
					sameName = false;
					break;
				}
			}
		}

		if (sameName)
		{
			inputText = firstName;
		}
		else
		{
			inputText = "...";
		}

		UI::PushId();
		if (UI::InputText("Name", inputText))
		{
			for (auto& entity : m_selectedEntites)
			{
				if (registry.HasComponent<Lamp::TagComponent>(entity))
				{
					registry.GetComponent<Lamp::TagComponent>(entity).tag = inputText;
				}
			}
		}
		UI::PopId();
	}

	// Transform
	{
		UI::PushId();
		if (UI::BeginProperties())
		{
			auto& entity = m_selectedEntites.front();

			if (registry.HasComponent<Lamp::TransformComponent>(entity))
			{
				auto& transform = registry.GetComponent<Lamp::TransformComponent>(entity);

				UI::PropertyAxisColor("Position", transform.position);
				UI::PropertyAxisColor("Rotation", transform.rotation);
				UI::PropertyAxisColor("Scale", transform.scale, 1.f);
			}

			UI::EndProperties();
		}
		UI::PopId();
	}

	if (singleSelected)
	{
		auto& entity = m_selectedEntites[0];

		auto componentData = registry.GetComponents(entity);

		for (auto& [guid, data] : componentData)
		{
			const auto& registryInfo = Wire::ComponentRegistry::GetRegistryDataFromGUID(guid);

			if (registryInfo.name == "TagComponent" || registryInfo.name == "TransformComponent")
			{
				continue;
			}

			if (ImGui::CollapsingHeader(registryInfo.name.c_str()))
			{
				UI::PushId();
				if (UI::BeginProperties())
				{
					for (auto& prop : registryInfo.properties)
					{
						if (!prop.visible)
						{
							continue;
						}

						switch (prop.type)
						{
							case Wire::ComponentRegistry::PropertyType::Bool: UI::Property(prop.name, *(bool*)(&data[prop.offset])); break;
							case Wire::ComponentRegistry::PropertyType::String: UI::Property(prop.name, *(std::string*)(&data[prop.offset])); break;

							case Wire::ComponentRegistry::PropertyType::Int: UI::Property(prop.name, *(int32_t*)(&data[prop.offset])); break;
							case Wire::ComponentRegistry::PropertyType::UInt: UI::Property(prop.name, *(uint32_t*)(&data[prop.offset])); break;

							case Wire::ComponentRegistry::PropertyType::Short: UI::Property(prop.name, *(int16_t*)(&data[prop.offset])); break;
							case Wire::ComponentRegistry::PropertyType::UShort: UI::Property(prop.name, *(uint16_t*)(&data[prop.offset])); break;

							case Wire::ComponentRegistry::PropertyType::Char: UI::Property(prop.name, *(int8_t*)(&data[prop.offset])); break;
							case Wire::ComponentRegistry::PropertyType::UChar: UI::Property(prop.name, *(uint8_t*)(&data[prop.offset])); break;

							case Wire::ComponentRegistry::PropertyType::Float: UI::Property(prop.name, *(float*)(&data[prop.offset])); break;
							case Wire::ComponentRegistry::PropertyType::Double: UI::Property(prop.name, *(double*)(&data[prop.offset])); break;

							case Wire::ComponentRegistry::PropertyType::Vector2: UI::Property(prop.name, *(glm::vec2*)(&data[prop.offset])); break;
							case Wire::ComponentRegistry::PropertyType::Vector3: UI::Property(prop.name, *(glm::vec3*)(&data[prop.offset])); break;
							case Wire::ComponentRegistry::PropertyType::Vector4: UI::Property(prop.name, *(glm::vec4*)(&data[prop.offset])); break;

							case Wire::ComponentRegistry::PropertyType::Color3: UI::PropertyColor(prop.name, *(glm::vec3*)(&data[prop.offset])); break;
							case Wire::ComponentRegistry::PropertyType::Color4: UI::PropertyColor(prop.name, *(glm::vec4*)(&data[prop.offset])); break;

							case Wire::ComponentRegistry::PropertyType::AssetHandle: UI::Property(prop.name, *(Lamp::AssetHandle*)(&data[prop.offset])); break;
						}
					}

					UI::EndProperties();
				}
				UI::PopId();
			}
		}

		registry.SetComponents(componentData, entity);

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.2f, 0.2f, 1.f });
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.f);
		if (ImGui::Button("Add Component"))
		{
			ImGui::OpenPopup("AddComponent");
		}
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();

		AddComponentPopup();
	}
}

void PropertiesPanel::AddComponentPopup()
{
	if (ImGui::BeginPopup("AddComponent"))
	{
		const auto& componentInfo = Wire::ComponentRegistry::ComponentGUIDs();
		for (const auto& [name, info] : componentInfo)
		{
			if (ImGui::MenuItem(name.c_str()))
			{
				for (auto& ent : m_selectedEntites)
				{
					if (!m_currentScene->GetRegistry().HasComponent(info.guid, ent))
					{
						std::vector<uint8_t> emptyData(info.size, 0);
						m_currentScene->GetRegistry().AddComponent(emptyData, info.guid, ent);
					}
				}

				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::EndPopup();
	}
}
