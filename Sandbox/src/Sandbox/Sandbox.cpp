#include "sbpch.h"
#include "Sandbox.h"

#include "Sandbox/Window/ViewportPanel.h"
#include "Sandbox/Window/PropertiesPanel.h"
#include "Sandbox/Window/MaterialEditorPanel.h"
#include "Sandbox/Window/EditorIconLibrary.h"
#include "Sandbox/Window/AssetBrowserPanel.h"
#include "Sandbox/Window/CreatePanel.h"
#include "Sandbox/Window/RenderPipelineEditorPanel.h"
#include "Sandbox/Window/RenderPassEditorPanel.h"
#include "Sandbox/Window/RenderGraphEditorPanel.h"

#include <Lamp/Rendering/SceneRenderer.h>
#include <Lamp/Rendering/Camera/EditorCameraController.h>

#include <Lamp/Components/Components.h>
#include <Lamp/Asset/AssetManager.h>
#include <Lamp/Asset/Mesh/Mesh.h>

#include <Lamp/Scene/Entity.h>
#include <Lamp/Scene/Scene.h>

#include <Lamp/Input/KeyCodes.h>
#include <Lamp/Input/Input.h>

#include <imgui.h>

Sandbox::~Sandbox()
{}

void Sandbox::OnAttach()
{
	EditorIconLibrary::Initialize();

	m_editorCameraController = new Lamp::EditorCameraController(60.f, 0.1f, 1000.f);
	m_editorScene = CreateRef<Lamp::Scene>("Scene");
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

			m_selectedEntities.emplace_back(entity);
		}
	}

	// Light
	{
		auto entity = m_editorScene->CreateEntity();

		auto& lightComp = entity.AddComponent<Lamp::DirectionalLightComponent>();
		lightComp.color = { 1.f, 1.f, 1.f };
		lightComp.intensity = 1.f;
		
		entity.AddComponent<Lamp::TransformComponent>();
	}

	m_editorWindows.emplace_back(CreateRef<ViewportPanel>(m_sceneRenderer, m_editorCameraController));
	m_editorWindows.emplace_back(CreateRef<PropertiesPanel>(m_selectedEntities, m_editorScene));
	m_editorWindows.emplace_back(CreateRef<MaterialEditorPanel>());
	m_editorWindows.emplace_back(CreateRef<AssetBrowserPanel>());
	m_editorWindows.emplace_back(CreateRef<CreatePanel>(m_selectedEntities, m_editorScene));
	m_editorWindows.emplace_back(CreateRef<RenderPipelineEditorPanel>());
	m_editorWindows.emplace_back(CreateRef<RenderPassEditorPanel>());
	m_editorWindows.emplace_back(CreateRef<RenderGraphEditorPanel>());
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

void Sandbox::ExecuteUndo()
{
	EditorCommandStack* cmdStack = nullptr;

	for (const auto window : m_editorWindows)
	{
		if (window->IsFocused())
		{
			cmdStack = &window->GetCommandStack();
			break;
		}
	}

	if (cmdStack)
	{
		cmdStack->Undo();
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

bool Sandbox::OnKeyPressedEvent(Lamp::KeyPressedEvent& e)
{
	const bool ctrlPressed = Lamp::Input::IsKeyDown(LP_KEY_LEFT_SHIFT);

	switch (e.GetKeyCode())
	{
		case LP_KEY_Z:
		{
			if (ctrlPressed)
			{
				ExecuteUndo();
			}

			break;
		}

		default:
			break;
	}

	return false;
}
