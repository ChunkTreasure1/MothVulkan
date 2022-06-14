#include "sbpch.h"
#include "Sandbox.h"

#include "Sandbox/Window/ViewportPanel.h"
#include "Sandbox/Window/PropertiesPanel.h"

#include <Lamp/Rendering/SceneRenderer.h>
#include <Lamp/Rendering/Camera/EditorCameraController.h>

#include <Lamp/Components/Components.h>
#include <Lamp/Asset/AssetManager.h>
#include <Lamp/Asset/Mesh/Mesh.h>

#include <Wire/SceneSystem/Scene.h>
#include <Wire/SceneSystem/Entity.h>

#include <imgui.h>

Sandbox::~Sandbox()
{
}

void Sandbox::OnAttach()
{
	m_editorCameraController = new Lamp::EditorCameraController(60.f, 0.1f, 100.f);
	m_editorScene = CreateRef<Scene>("Scene");
	m_sceneRenderer = CreateRef<Lamp::SceneRenderer>(m_editorScene, "Engine/RenderGraph/renderGraph.lprg");

	for (uint32_t i = 0; i < 10; i++)
	{
		for (uint32_t j = 0; j < 10; j++)
		{
			auto entity = m_editorScene->CreateEntity();
			auto& mesh = entity.AddComponent<Lamp::MeshComponent>();
			mesh.handle = Lamp::AssetManager::GetHandle<Lamp::Mesh>("Assets/SM_Particle_Chest.fbx");

			auto& transform = entity.AddComponent<Lamp::TransformComponent>();
			transform.scale = { 0.01f, 0.01f, 0.01f };
			transform.position = { i, 0.f, j };
			
			auto& tag = entity.AddComponent<Lamp::TagComponent>();
			tag.tag = "Entity";
		
			if (i == 0 && j == 0)
			{
				m_selectedEntities.emplace_back(entity);
			}
		}
	}
	 
	m_editorWindows.emplace_back(CreateRef<ViewportPanel>(m_sceneRenderer));
	m_editorWindows.emplace_back(CreateRef<PropertiesPanel>(m_selectedEntities));
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
