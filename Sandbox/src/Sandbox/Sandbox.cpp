#include "sbpch.h"
#include "Sandbox.h"

#include "Sandbox/Window/ViewportPanel.h"
#include "Sandbox/Window/PropertiesPanel.h"
#include "Sandbox/Window/MaterialEditorPanel.h"
#include "Sandbox/Window/EditorIconLibrary.h"
#include "Sandbox/Window/AssetBrowserPanel.h"

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
	EditorIconLibrary::Initialize();

	m_editorCameraController = new Lamp::EditorCameraController(60.f, 0.1f, 1000.f);
	m_editorScene = CreateRef<Scene>("Scene");
	m_sceneRenderer = CreateRef<Lamp::SceneRenderer>(m_editorScene, "Engine/RenderGraph/renderGraph.lprg");

	for (uint32_t i = 0; i < 1; i++)
	{
		for (uint32_t j = 0; j < 1; j++)
		{
			auto entity = m_editorScene->CreateEntity();
			auto& mesh = entity.AddComponent<Lamp::MeshComponent>();
			mesh.handle = Lamp::AssetManager::GetHandle<Lamp::Mesh>("Assets/Meshes/Sponza/Sponza2022.glb");

			auto& transform = entity.AddComponent<Lamp::TransformComponent>();
			transform.scale = { 1.f, 1.f, 1.f };
			transform.position = { i * 40, 0.f, j * 40 };

			auto& tag = entity.AddComponent<Lamp::TagComponent>();
			tag.tag = "Entity";
		}
	}

	// Light
	{
		auto entity = m_editorScene->CreateEntity();

		auto& lightComp = entity.AddComponent<Lamp::DirectionalLightComponent>();
		lightComp.color = { 1.f, 1.f, 1.f };
		lightComp.intensity = 1.f;
		
		entity.AddComponent<Lamp::TransformComponent>();
		m_selectedEntities.emplace_back(entity);
	}

	m_editorWindows.emplace_back(CreateRef<ViewportPanel>(m_sceneRenderer, m_editorCameraController));
	m_editorWindows.emplace_back(CreateRef<PropertiesPanel>(m_selectedEntities));
	m_editorWindows.emplace_back(CreateRef<MaterialEditorPanel>());
	m_editorWindows.emplace_back(CreateRef<AssetBrowserPanel>());
}

void Sandbox::OnDetach()
{
	m_editorWindows.clear();
	delete m_editorCameraController;
	m_editorCameraController = nullptr;

	EditorIconLibrary::Shutdown();
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
			window->UpdateMainContent();
			window->End();

			window->UpdateContent();
		}
	}

	return false;
}

bool Sandbox::OnRenderEvent(Lamp::AppRenderEvent& e)
{
	m_sceneRenderer->OnRender(m_editorCameraController->GetCamera());

	return false;
}