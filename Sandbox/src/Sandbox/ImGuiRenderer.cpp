#include "sbpch.h"
#include "Sandbox.h"

#include "Sandbox/Window/EditorWindow.h"

#include <Lamp/Asset/AssetManager.h>

#include <imgui.h>

void Sandbox::UpdateDockSpace()
{
	static bool opt_fullscreen_persistant = true;
	bool opt_fullscreen = opt_fullscreen_persistant;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}

	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;

	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
	// all active windows docked into it will lose their parent and become undocked.
	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	bool pp = true;
	bool* p = &pp;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("MainDockspace", p, window_flags);
	ImGui::PopStyleVar();

	if (opt_fullscreen)
		ImGui::PopStyleVar(2);

	// DockSpace
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowMinSize.x = 200.f;

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New scene", "Ctrl + N"))
			{
				NewScene();
			}

			if (ImGui::MenuItem("Open scene", "Ctrl + O"))
			{
				OpenScene();
			}

			if (ImGui::MenuItem("Save scene as", "Ctrl + Shift + S"))
			{
				SaveSceneAs();
			}

			if (ImGui::MenuItem("Save", "Ctrl + S"))
			{
				SaveScene();
			}

			if (ImGui::MenuItem("Save Scene"))
			{
				if (m_editorScene)
				{
					m_editorScene->path = "Assets/Scenes/TestScene";
					Lamp::AssetManager::Get().SaveAsset(m_editorScene);
				}
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "Ctrl + Z"))
			{
				ExecuteUndo();
			}

			if (ImGui::MenuItem("Redo", "Ctrl + Y"))
			{
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tools"))
		{
			for (auto window : m_editorWindows)
			{
				ImGui::MenuItem(window->GetTitle().c_str(), "", &const_cast<bool&>(window->IsOpen()));
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	ImGui::End();
}