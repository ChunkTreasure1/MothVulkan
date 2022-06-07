#include "sbpch.h"
#include "Sandbox.h"

#include "Sandbox/Window/ViewportPanel.h"

#include <imgui.h>

void Sandbox::OnAttach()
{
	m_editorWindows.emplace_back(CreateRef<ViewportPanel>());
}

void Sandbox::OnDetach()
{
	m_editorWindows.clear();
}

void Sandbox::OnEvent(Lamp::Event& e)
{
	Lamp::EventDispatcher dispatcher(e);
	dispatcher.Dispatch<Lamp::AppImGuiUpdateEvent>(LP_BIND_EVENT_FN(Sandbox::OnImGuiUpdateEvent));

	for (auto& window : m_editorWindows)
	{
		window->OnEvent(e);
	}
}

bool Sandbox::OnImGuiUpdateEvent(Lamp::AppImGuiUpdateEvent& e)
{
	UpdateDockSpace();

	for (auto& window : m_editorWindows)
	{
		if (window->Begin())
		{
			window->UpdateContent();
			window->End();
		}
	}

	return false;
}