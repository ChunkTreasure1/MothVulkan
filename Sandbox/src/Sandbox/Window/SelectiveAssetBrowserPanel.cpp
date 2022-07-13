#include "sbpch.h"
#include "SelectiveAssetBrowserPanel.h"

#include "Sandbox/Window/EditorIconLibrary.h"

#include <Lamp/Utility/FileSystem.h>
#include <Lamp/Asset/AssetManager.h>
#include <Lamp/Utility/UIUtility.h>

static uint32_t s_assetBrowserCount;

SelectiveAssetBrowserPanel::SelectiveAssetBrowserPanel(Lamp::AssetType assetType)
	: EditorWindow("Asset Browser##" + std::to_string(s_assetBrowserCount++)), m_selectiveAssetType(assetType)
{
	UpdateAssetList();
}

void SelectiveAssetBrowserPanel::UpdateMainContent()
{
	RenderControlsBar();

	ImGui::BeginChild("##view", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetWindowHeight()));
	{
		ImGui::BeginChild("Scrolling");
		{
			const float cellSize = m_thumbnailSize + m_thumbnailPadding;
			const float panelWidth = ImGui::GetContentRegionAvail().x;
			int32_t columnCount = (int32_t)(panelWidth / cellSize);

			if (columnCount < 1)
			{
				columnCount = 1;
			}

			ImGui::Columns(columnCount, nullptr, false);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.f, 0.f, 0.f, 0.f });

			for (const auto& asset : m_allAssetsOfType)
			{
				ImGui::PushID(asset.path.filename().string().c_str());

				Ref<Lamp::Texture2D> icon = EditorIconLibrary::GetIcon(EditorIcon::GenericFile);

				UI::ImageButton(asset.path.filename().string(), UI::GetTextureID(icon), { m_thumbnailSize, m_thumbnailSize });

				ImGui::TextWrapped(asset.path.filename().string().c_str());
				ImGui::NextColumn();

				ImGui::PopID();
			}

			ImGui::PopStyleColor();
		}
		ImGui::EndChild();
	}
	ImGui::EndChild();
}

void SelectiveAssetBrowserPanel::UpdateAssetList()
{
	m_allAssetsOfType.clear();

	for (auto it : std::filesystem::recursive_directory_iterator(FileSystem::GetEnginePath()))
	{
		if (!it.is_directory())
		{
			const std::filesystem::path path = it.path();
			const Lamp::AssetType type = Lamp::AssetManager::Get().GetAssetTypeFromPath(path);

			if (type == m_selectiveAssetType)
			{
				auto& data = m_allAssetsOfType.emplace_back();
				data.path = path;
				data.handle = Lamp::AssetManager::Get().GetAssetHandleFromPath(path);
				data.type = Lamp::AssetManager::Get().GetAssetTypeFromPath(path);
			}
		}
	}
}

void SelectiveAssetBrowserPanel::RenderControlsBar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
	
	UI::ScopedColor childColor{ ImGuiCol_ChildBg, { 0.2f, 0.2f, 0.2f, 1.f } };
	ImGui::BeginChild("##controlsBar", { 0.f, m_controlsBarHeight });
	{
		const float buttonSizeOffset = 10.f;
		
		UI::ShiftCursor(5.f, 4.f);
		{
			UI::ScopedColor buttonBackground(ImGuiCol_Button, { 0.f, 0.f, 0.f, 0.f });
		
			if (UI::ImageButton("##reloadButton", UI::GetTextureID(EditorIconLibrary::GetIcon(EditorIcon::Reload)), { m_controlsBarHeight - buttonSizeOffset, m_controlsBarHeight - buttonSizeOffset }))
			{
				UpdateAssetList();
			}
		}
	}
	ImGui::EndChild();

	ImGui::PopStyleVar();
}
