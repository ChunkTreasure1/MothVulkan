#include "sbpch.h"
#include "PropertiesPanel.h"

#include <Lamp/Components/Components.h>
#include <Lamp/Utility/UIUtility.h>

PropertiesPanel::PropertiesPanel(std::vector<Entity>& selectedEntites)
	: EditorWindow("Properties"), m_selectedEntites(selectedEntites)
{
	m_isOpen = true;
}

void PropertiesPanel::UpdateContent()
{
	if (m_selectedEntites.empty())
	{
		return;
	}

	const bool singleSelected = !(m_selectedEntites.size() > 1);

	if (singleSelected)
	{
		Entity& entity = m_selectedEntites[0];
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

	if (singleSelected)
	{
		auto& entity = m_selectedEntites[0];
	}
}