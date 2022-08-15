#pragma once

#include "Sandbox/EditorCommandStack.h"

#include <Lamp/Event/Event.h>
#include <imgui.h>

#include <string>

namespace Lamp
{
	class Asset;
}

class EditorWindow
{
public:
	EditorWindow(const std::string& title, bool dockSpace = false);
	virtual ~EditorWindow() = default;

	bool Begin();
	void End();

	virtual void UpdateMainContent() = 0;
	virtual void UpdateContent() {}
	virtual void OnEvent(Lamp::Event& e) {}
	virtual void Open(Ref<Lamp::Asset> asset) {}

	inline const std::string& GetTitle() const { return m_title; }
	inline const bool& IsOpen() const { return m_isOpen; }
	inline const bool IsFocused() const { return m_isFocused; }

	inline EditorCommandStack& GetCommandStack() { return m_commandStack; }

protected:
	std::string m_title;
	EditorCommandStack m_commandStack{};

	ImGuiWindowFlags m_windowFlags = 0;

	bool m_isOpen = false;
	bool m_isFocused = false;
	bool m_isHovered = false;
	bool m_hasDockSpace = false;
};