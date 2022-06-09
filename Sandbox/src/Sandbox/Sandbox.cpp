#include "sbpch.h"
#include "Sandbox.h"

#include "Sandbox/Window/ViewportPanel.h"

#include <Lamp/Rendering/SceneRenderer.h>
#include <Lamp/Rendering/Camera/EditorCameraController.h>
#include <Wire/SceneSystem/Scene.h>

#include <imgui.h>

Sandbox::~Sandbox()
{
}

void Sandbox::OnAttach()
{
	m_editorCameraController = new Lamp::EditorCameraController(60.f, 0.1f, 100.f);
	m_editorScene = CreateRef<Scene>("Scene");
	m_sceneRenderer = CreateRef<Lamp::SceneRenderer>(m_editorScene, "Engine/RenderGraph/renderGraph.lprg");

	m_editorWindows.emplace_back(CreateRef<ViewportPanel>(m_sceneRenderer->GetFinalFramebuffer()));
}

void Sandbox::OnDetach()
{
	m_editorWindows.clear();
	delete m_editorCameraController;
	m_editorCameraController = nullptr;
}

void Sandbox::OnEvent(Lamp::Event& e)
{
	Lamp::EventDispatcher dispatcher(e);
	dispatcher.Dispatch<Lamp::AppImGuiUpdateEvent>(LP_BIND_EVENT_FN(Sandbox::OnImGuiUpdateEvent));
	dispatcher.Dispatch<Lamp::AppRenderEvent>(LP_BIND_EVENT_FN(Sandbox::OnRenderEvent));

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

bool Sandbox::OnRenderEvent(Lamp::AppRenderEvent& e)
{
	m_sceneRenderer->OnRender(m_editorCameraController->GetCamera());

	return false;
}
