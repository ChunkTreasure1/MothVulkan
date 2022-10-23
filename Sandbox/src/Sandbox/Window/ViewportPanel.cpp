#include "sbpch.h"
#include "ViewportPanel.h"

#include <Lamp/Asset/Mesh/Mesh.h>
#include <Lamp/Components/Components.h>

#include <Lamp/Rendering/Camera/EditorCameraController.h>
#include <Lamp/Rendering/SceneRenderer.h>
#include <Lamp/Rendering/Framebuffer.h>
#include <Lamp/Rendering/Camera/Camera.h>

#include <Lamp/Scene/Entity.h>
#include <Lamp/Utility/UIUtility.h>
#include <Lamp/Utility/Math.h>

ViewportPanel::ViewportPanel(Ref<Lamp::SceneRenderer> sceneRenderer, Ref<Lamp::Scene>& editorScene, Lamp::EditorCameraController* cameraController, std::vector<Wire::EntityId>& selectedEntites,
	ImGuizmo::OPERATION& gizmoOperation, ImGuizmo::MODE& gizmoMode)
	: EditorWindow("Viewport"), m_sceneRenderer(sceneRenderer), m_editorCameraController(cameraController), m_editorScene(editorScene), m_selectedEntites(selectedEntites),
	m_gizmoOperation(gizmoOperation), m_gizmoMode(gizmoMode)
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
				auto mesh = Lamp::AssetManager::QueueAsset<Lamp::Mesh>(handle);
				meshComp.handle = mesh->handle;

				break;
			}

			case Lamp::AssetType::MeshSource:
			{
				Lamp::Entity newEntity = m_editorScene->CreateEntity();

				auto& meshComp = newEntity.AddComponent<Lamp::MeshComponent>();
				auto mesh = Lamp::AssetManager::QueueAsset<Lamp::Mesh>(handle);
				meshComp.handle = mesh->handle;

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

	// Gizmo
	{
		static glm::mat4 transform = glm::mat4(1.f);
		static glm::mat4 startTransform = glm::mat4(1.f);

		if (!m_selectedEntites.empty())
		{
			auto& transformComp = m_editorScene->GetRegistry().GetComponent<Lamp::TransformComponent>(m_selectedEntites[0]);

			const glm::mat4 currTransform = glm::translate(glm::mat4(1.f), transformComp.position) *
				glm::rotate(glm::mat4(1.f), glm::radians(transformComp.rotation.x), glm::vec3(1, 0, 0)) *
				glm::rotate(glm::mat4(1.f), glm::radians(transformComp.rotation.y), glm::vec3(0, 1, 0)) *
				glm::rotate(glm::mat4(1.f), glm::radians(transformComp.rotation.z), glm::vec3(0, 0, 1)) *
				glm::scale(glm::mat4(1.f), transformComp.scale);

			transform = currTransform;

			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();

			ImGuizmo::SetRect(m_perspectiveBounds[0].x, m_perspectiveBounds[0].y, m_perspectiveBounds[1].x - m_perspectiveBounds[0].x, m_perspectiveBounds[1].y - m_perspectiveBounds[0].y);
			ImGuizmo::Manipulate(
				glm::value_ptr(m_editorCameraController->GetCamera()->GetView()),
				glm::value_ptr(m_editorCameraController->GetCamera()->GetProjection()),
				m_gizmoOperation, m_gizmoMode, glm::value_ptr(transform));

			if (currTransform != transform)
			{
				glm::vec3 p, r, s;
				Lamp::Math::DecomposeTransform(transform, p, r, s);

				glm::vec3 deltaRot = r - transformComp.rotation;

				transformComp.position = p;
				transformComp.rotation += deltaRot;
				transformComp.scale = s;
			}

		}
	}

	ImGui::PopStyleVar(2);
}

void ViewportPanel::UpdateToolbar(float toolbarHeight, float toolbarXPadding)
{
}
