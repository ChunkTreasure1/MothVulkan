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
#include "Sandbox/Window/SceneViewPanel.h"
#include "Sandbox/Window/EditorSettingsPanel.h"
#include "Sandbox/Window/LogPanel.h"

#include "Sandbox/Window/EditorLibrary.h"

#include <Lamp/Rendering/SceneRenderer.h>
#include <Lamp/Rendering/Camera/EditorCameraController.h>

#include <Lamp/Components/Components.h>
#include <Lamp/Asset/AssetManager.h>
#include <Lamp/Asset/Mesh/Mesh.h>

#include <Lamp/Scene/Entity.h>
#include <Lamp/Scene/Scene.h>

#include <Lamp/Input/KeyCodes.h>
#include <Lamp/Input/MouseButtonCodes.h>
#include <Lamp/Input/Input.h>

#include <Lamp/Utility/FileSystem.h>

Sandbox::~Sandbox()
{
}

void Sandbox::OnAttach()
{
	EditorIconLibrary::Initialize();
	VersionControl::Initialize(VersionControlSystem::Perforce);

	m_editorCameraController = new Lamp::EditorCameraController(60.f, 0.1f, 1000.f);
	m_editorScene = CreateRef<Lamp::Scene>("Scene");
	m_sceneRenderer = CreateRef<Lamp::SceneRenderer>(m_editorScene, "Engine/RenderGraph/renderGraph.lprg");

	//// Sponza 
	//{
	//	auto entity = m_editorScene->CreateEntity();
	//	auto& mesh = entity.AddComponent<Lamp::MeshComponent>();
	//	mesh.handle = Lamp::AssetManager::GetHandle<Lamp::Mesh>("Assets/Meshes/Sponza/Sponza2022.glb");
	//}

	// Light
	{
		auto entity = m_editorScene->CreateEntity();
		entity.RemoveComponent<Lamp::TagComponent>();

		//auto& lightComp = entity.AddComponent<Lamp::DirectionalLightComponent>();
		//lightComp.color = { 1.f, 1.f, 1.f };
		//lightComp.intensity = 1.f;
	}

	m_editorWindows.emplace_back(CreateRef<ViewportPanel>(m_sceneRenderer, m_editorScene, m_editorCameraController, m_selectedEntities, m_gizmoOperation, m_gizmoMode));
	m_editorWindows.emplace_back(CreateRef<PropertiesPanel>(m_selectedEntities, m_editorScene));
	m_editorWindows.emplace_back(CreateRef<CreatePanel>(m_selectedEntities, m_editorScene));
	m_editorWindows.emplace_back(CreateRef<AssetBrowserPanel>());
	m_editorWindows.emplace_back(CreateRef<SceneViewPanel>(m_selectedEntities, m_editorScene));
	m_editorWindows.emplace_back(CreateRef<EditorSettingsPanel>(m_settings));
	m_editorWindows.emplace_back(CreateRef<LogPanel>());

	m_editorWindows.emplace_back(CreateRef<MaterialEditorPanel>());
	EditorLibrary::Register(Lamp::AssetType::Material, m_editorWindows.back());

	m_editorWindows.emplace_back(CreateRef<RenderPipelineEditorPanel>());
	EditorLibrary::Register(Lamp::AssetType::RenderPipeline, m_editorWindows.back());

	m_editorWindows.emplace_back(CreateRef<RenderPassEditorPanel>());
	EditorLibrary::Register(Lamp::AssetType::RenderPass, m_editorWindows.back());

	m_editorWindows.emplace_back(CreateRef<RenderGraphEditorPanel>());
	EditorLibrary::Register(Lamp::AssetType::RenderGraph, m_editorWindows.back());

	Lamp::Application::Get().GetWindow()->Maximize();
}

void Sandbox::OnDetach()
{
	m_editorWindows.clear();
	delete m_editorCameraController;
	m_editorCameraController = nullptr;

	VersionControl::Shutdown();
	EditorIconLibrary::Shutdown();
}

void Sandbox::OnEvent(Lamp::Event& e)
{
	Lamp::EventDispatcher dispatcher(e);
	dispatcher.Dispatch<Lamp::AppImGuiUpdateEvent>(LP_BIND_EVENT_FN(Sandbox::OnImGuiUpdateEvent));
	dispatcher.Dispatch<Lamp::AppRenderEvent>(LP_BIND_EVENT_FN(Sandbox::OnRenderEvent));
	dispatcher.Dispatch<Lamp::KeyPressedEvent>(LP_BIND_EVENT_FN(Sandbox::OnKeyPressedEvent));

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

void Sandbox::NewScene()
{
	SaveScene();
	m_editorScene = CreateRef<Lamp::Scene>("New Scene");
	m_sceneRenderer->SetScene(m_editorScene);
}

void Sandbox::OpenScene()
{
	const std::filesystem::path loadPath = FileSystem::OpenFile("Scene (*.lpscene)\0*.lpscene\0");
	if (!loadPath.empty() && FileSystem::Exists(loadPath))
	{
		m_editorScene = Lamp::AssetManager::GetAsset<Lamp::Scene>(loadPath);
		m_sceneRenderer->SetScene(m_editorScene);
	}
}

void Sandbox::SaveScene()
{
	if (m_editorScene)
	{
		if (!m_editorScene->path.empty())
		{
			Lamp::AssetManager::Get().SaveAsset(m_editorScene);
		}
		else
		{
			SaveSceneAs();
		}
	}
}

void Sandbox::SaveSceneAs()
{
	std::filesystem::path savePath = FileSystem::OpenFolder();
	if (!savePath.empty() && FileSystem::Exists(savePath))
	{
		m_editorScene->path = savePath;
		Lamp::AssetManager::Get().SaveAsset(m_editorScene);
	}
}

bool Sandbox::OnImGuiUpdateEvent(Lamp::AppImGuiUpdateEvent& e)
{
	ImGuizmo::BeginFrame();

	ShouldSavePopup();
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
	const bool ctrlPressed = Lamp::Input::IsKeyDown(LP_KEY_LEFT_CONTROL);
	const bool shiftPressed = Lamp::Input::IsKeyDown(LP_KEY_LEFT_SHIFT);

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

		case LP_KEY_S:
		{
			if (ctrlPressed && !shiftPressed)
			{
				SaveScene();
			}
			else if (ctrlPressed && shiftPressed)
			{
				SaveSceneAs();
			}

			break;
		}

		case LP_KEY_O:
		{
			if (ctrlPressed)
			{
				if (m_editorScene)
				{
					ImGui::OpenPopup("OpenSavePopup");
				}
				else
				{
					OpenScene();
				}
			}

			break;
		}

		case LP_KEY_N:
		{
			if (ctrlPressed)
			{
				NewScene();
			}

			break;
		}

		case LP_KEY_W:
		{
			if (!Lamp::Input::IsMouseButtonPressed(LP_MOUSE_BUTTON_RIGHT))
			{
				m_gizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
			}
			break;
		}

		case LP_KEY_E:
		{
			if (!Lamp::Input::IsMouseButtonPressed(LP_MOUSE_BUTTON_RIGHT))
			{
				m_gizmoOperation = ImGuizmo::OPERATION::ROTATE;
			}
			break;
		}

		case LP_KEY_R:
		{
			if (!Lamp::Input::IsMouseButtonPressed(LP_MOUSE_BUTTON_RIGHT))
			{
				m_gizmoOperation = ImGuizmo::OPERATION::SCALE;
			}
			break;
		}

		default:
			break;
	}

	return false;
}
