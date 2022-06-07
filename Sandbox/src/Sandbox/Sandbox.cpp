#include "sbpch.h"
#include "Sandbox.h"


#include <imgui.h>

Sandbox::Sandbox()
{
}

void Sandbox::OnEvent(Lamp::Event& e)
{
	Lamp::EventDispatcher dispatcher(e);
	dispatcher.Dispatch<Lamp::AppImGuiUpdateEvent>(LP_BIND_EVENT_FN(Sandbox::OnImGuiUpdateEvent));
}

bool Sandbox::OnImGuiUpdateEvent(Lamp::AppImGuiUpdateEvent& e)
{
	UpdateDockSpace();

	ImGui::ShowDemoWindow();

	return false;
}