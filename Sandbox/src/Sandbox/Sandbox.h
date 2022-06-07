#pragma once

#include <Lamp/Event/ApplicationEvent.h>

class Sandbox
{
public:
	Sandbox();

	void OnEvent(Lamp::Event& e);

private:
	bool OnImGuiUpdateEvent(Lamp::AppImGuiUpdateEvent& e);

	/////ImGui/////
	void UpdateDockSpace();
	///////////////
};
