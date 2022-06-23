#include "sbpch.h"
#include "PropertiesPanel.h"

#include <Lamp/Components/Components.h>
#include <Lamp/Utility/UIUtility.h>

PropertiesPanel::PropertiesPanel(std::vector<Lamp::Entity>& selectedEntites)
	: EditorWindow("Properties"), m_selectedEntites(selectedEntites)
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

	if (singleSelected)
	{
		Lamp::Entity& entity = m_selectedEntites[0];
		if (entity.HasComponent<Lamp::TagComponent>())
		{
			auto& tag = entity.GetComponent<Lamp::TagComponent>();
			UI::InputText("Name", tag.tag);
		}
	}
	else
	{
		static std::string inputText = "...";

		std::string firstName;
		bool sameName = true;

		if (m_selectedEntites[0].HasComponent<Lamp::TagComponent>())
		{
			firstName = m_selectedEntites[0].GetComponent<Lamp::TagComponent>().tag;
		}

		for (auto& entity : m_selectedEntites)
		{
			if (entity.HasComponent<Lamp::TagComponent>())
			{
				auto& tag = entity.GetComponent<Lamp::TagComponent>();
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
				if (entity.HasComponent<Lamp::TagComponent>())
				{
					entity.GetComponent<Lamp::TagComponent>().tag = inputText;
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

			if (entity.HasComponent<Lamp::TransformComponent>())
			{
				auto& transform = entity.GetComponent<Lamp::TransformComponent>();

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

		auto componentData = entity.GetComponents();

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
					size_t offset = 0;
					for (auto& prop : registryInfo.properties)
					{
						const size_t size = Wire::ComponentRegistry::GetSizeFromType(prop.type);
						switch (prop.type)
						{
							case Wire::ComponentRegistry::PropertyType::Bool: UI::Property(prop.name, *(bool*)(&data[offset])); break;
							case Wire::ComponentRegistry::PropertyType::String: UI::Property(prop.name, *(std::string*)(&data[offset])); break;

							case Wire::ComponentRegistry::PropertyType::Int: UI::Property(prop.name, *(int32_t*)(&data[offset])); break;
							case Wire::ComponentRegistry::PropertyType::UInt: UI::Property(prop.name, *(uint32_t*)(&data[offset])); break;

							case Wire::ComponentRegistry::PropertyType::Short: UI::Property(prop.name, *(int16_t*)(&data[offset])); break;
							case Wire::ComponentRegistry::PropertyType::UShort: UI::Property(prop.name, *(uint16_t*)(&data[offset])); break;

							case Wire::ComponentRegistry::PropertyType::Char: UI::Property(prop.name, *(int8_t*)(&data[offset])); break;
							case Wire::ComponentRegistry::PropertyType::UChar: UI::Property(prop.name, *(uint8_t*)(&data[offset])); break;

							case Wire::ComponentRegistry::PropertyType::Float: UI::Property(prop.name, *(float*)(&data[offset])); break;
							case Wire::ComponentRegistry::PropertyType::Double: UI::Property(prop.name, *(double*)(&data[offset])); break;

							case Wire::ComponentRegistry::PropertyType::Vector2: UI::Property(prop.name, *(glm::vec2*)(&data[offset])); break;
							case Wire::ComponentRegistry::PropertyType::Vector3: UI::Property(prop.name, *(glm::vec3*)(&data[offset])); break;
							case Wire::ComponentRegistry::PropertyType::Vector4: UI::Property(prop.name, *(glm::vec4*)(&data[offset])); break;
						}

						offset += size; 
					}

					UI::EndProperties();
				}
				UI::PopId();
			}
		}

		entity.SetComponents(componentData);
	}
}