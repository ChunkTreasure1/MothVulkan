#include "sbpch.h"
#include "ViewportPanel.h"

#include <Lamp/Rendering/Camera/EditorCameraController.h>

#include <Lamp/Rendering/SceneRenderer.h>
#include <Lamp/Rendering/Framebuffer.h>
#include <Lamp/Utility/UIUtility.h>

#include <Lamp/Asset/Mesh/Mesh.h>
#include <Lamp/Components/Components.h>
#include <Lamp/Scene/Entity.h>

ViewportPanel::ViewportPanel(Ref<Lamp::SceneRenderer> sceneRenderer, Ref<Lamp::Scene>& editorScene, Lamp::EditorCameraController* cameraController)
	: EditorWindow("Viewport"), m_sceneRenderer(sceneRenderer), m_editorCameraController(cameraController), m_editorScene(editorScene)
{
	m_isOpen = true;
	m_windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
}

void ViewportPanel::UpdateMainContent()
{
	auto size = ImGui::GetContentRegionAvail();

	auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
	auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
	auto viewportOffset = ImGui::GetWindowPos();

	m_perspectiveBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
	m_perspectiveBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

	const float toolBarSize = 22.f;
	const float toolBarYPadding = 5.f;
	const float toolBarXPadding = 5.f;

	ImVec2 viewportSize = ImGui::GetContentRegionAvail();
	viewportSize.y -= (toolBarSize + toolBarYPadding);

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 0.f, 0.f });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0.f, 0.f });

	/////Perspective toolbar/////
	{
		UI::ScopedColor childBg(ImGuiCol_ChildBg, { 0.258f, 0.258f, 0.258f, 1.000f });
		ImGui::BeginChild("toolbarChild", { ImGui::GetContentRegionAvail().x, toolBarSize + toolBarYPadding }, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		UI::ShiftCursor(toolBarXPadding, toolBarYPadding / 2.f);

		UpdateToolbar(toolBarSize, toolBarXPadding);
		ImGui::EndChild();
	}
	/////////////////////////////

	ImGui::PopStyleVar(2);

	if (m_viewportSize != (*(glm::vec2*)&viewportSize))
	{
		m_viewportSize = { viewportSize.x, viewportSize.y };
		m_sceneRenderer->Resize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);

		m_editorCameraController->UpdateProjection((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
	}

	ImGui::Image(UI::GetTextureID(m_sceneRenderer->GetFinalFramebuffer()->GetColorAttachment(0)), size);
	if (void* ptr = UI::DragDropTarget({ "ASSET_BROWSER_ITEM" }))
	{
		const Lamp::AssetHandle handle = *(const Lamp::AssetHandle*)ptr;

		Lamp::AssetType type = Lamp::AssetManager::Get().GetAssetTypeFromHandle(handle);

		switch (type)
		{
			case Lamp::AssetType::Mesh:
			{
				Lamp::Entity newEntity = m_editorScene->CreateEntity();

				auto& meshComp = newEntity.AddComponent<Lamp::MeshComponent>();
				auto mesh = Lamp::AssetManager::GetAsset<Lamp::Mesh>(handle);
				if (mesh)
				{
					meshComp.handle = mesh->handle;
				}

				break;
			}

			case Lamp::AssetType::MeshSource:
			{
				Lamp::Entity newEntity = m_editorScene->CreateEntity();

				auto& meshComp = newEntity.AddComponent<Lamp::MeshComponent>();
				auto mesh = Lamp::AssetManager::GetAsset<Lamp::Mesh>(handle);
				if (mesh)
				{
					meshComp.handle = mesh->handle;
				}

				break;
			}

			case Lamp::AssetType::Scene:
			{
				if (m_editorScene)
				{
					if (!m_editorScene->path.empty())
					{
						Lamp::AssetManager::Get().SaveAsset(m_editorScene);
					}
					else
					{
						std::filesystem::path savePath = FileSystem::OpenFolder();
						if (!savePath.empty() && FileSystem::Exists(savePath))
						{
							m_editorScene->path = savePath;
							Lamp::AssetManager::Get().SaveAsset(m_editorScene);
						}
					}
				}

				m_editorScene = Lamp::AssetManager::GetAsset<Lamp::Scene>(handle);

				break;
			}
		}
	}
}

void ViewportPanel::UpdateToolbar(float toolbarHeight, float toolbarXPadding)
{
}
