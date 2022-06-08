#include "sbpch.h"
#include "Sandbox.h"

#include "Sandbox/Window/ViewportPanel.h"
#include "Sandbox/Editor/EditorCameraController.h"

#include <Lamp/Rendering/SceneRenderer.h>
#include <Wire/SceneSystem/Scene.h>

#include <imgui.h>

void Sandbox::OnAttach()
{
	m_editorCameraController = CreateRef<EditorCameraController>(60.f, 0.1f, 100.f);
	m_editorScene = CreateRef<Scene>("Scene");
	m_sceneRenderer = CreateRef<Lamp::SceneRenderer>(m_editorScene);

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

	m_editorCameraController->OnEvent(e);

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