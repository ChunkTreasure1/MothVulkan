#pragma once

#include <Lamp/Event/Event.h>

#include <imgui.h>

#include <string>

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

	inline const std::string& GetTitle() const { return m_title; }
	inline const bool IsOpen() const { return m_isOpen; }
	inline const bool IsFocused() const { return m_isFocused; }

protected:
	std::string m_title;
	
	ImGuiWindowFlags m_windowFlags = 0;

	bool m_isOpen = false;
	bool m_isFocused = false;
	bool m_isHovered = false;
	bool m_hasDockSpace = false;
};