#include "sbpch.h"
#include "CreatePanel.h"

#include "Sandbox/Window/EditorIconLibrary.h"

#include <Lamp/Utility/UIUtility.h>
#include <Lamp/Components/Components.h>

CreatePanel::CreatePanel(std::vector<Wire::EntityId>& selectedEntites, Ref<Lamp::Scene>& scene)
	: EditorWindow("Create"), m_selectedEntites(selectedEntites), m_scene(scene)
{
	m_windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	m_isOpen = true;

	m_directories[FileSystem::GetAssetsPath().string()] = ProcessDirectory(FileSystem::GetAssetsPath().string(), nullptr);
	m_currentDirectory = m_directories[FileSystem::GetAssetsPath().string()].get();
}

void CreatePanel::UpdateMainContent()
{
	UI::ScopedColor buttonColor(ImGuiCol_Button, { 0.313f, 0.313f, 0.313f, 1.f });
	UI::ScopedStyleFloat buttonRounding(ImGuiStyleVar_FrameRounding, 2.f);

	if (!m_meshListOpen)
	{
		const ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit;
		if (ImGui::BeginTable("createButtons", 2, tableFlags))
		{
			ImGui::TableSetupColumn("Column1", 0, ImGui::GetContentRegionAvail().x / 2.f);
			ImGui::TableSetupColumn("Column2", 0, ImGui::GetContentRegionAvail().x / 2.f);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			if (ImGui::Button("Entity", { ImGui::GetContentRegionAvail().x, m_buttonSize }))
			{
				Lamp::Entity newEntity = m_scene->CreateEntity();

				m_selectedEntites.clear();
				m_selectedEntites.emplace_back(newEntity.GetId());
			}

			ImGui::TableNextColumn();

			if (ImGui::Button("Mesh", { ImGui::GetContentRegionAvail().x, m_buttonSize }))
			{
				m_meshListOpen = true;
			}

			ImGui::EndTable();
		}
	}
	else
	{

		if (UI::ImageButton("##backButton", UI::GetTextureID(EditorIconLibrary::GetIcon(EditorIcon::Back)), { m_buttonSize, m_buttonSize }))
		{
			m_meshListOpen = false;
		}

		ImGui::SameLine();

		if (UI::ImageButton("##reloadButton", UI::GetTextureID(EditorIconLibrary::GetIcon(EditorIcon::Reload)), { m_buttonSize, m_buttonSize }))
		{

		}

		ImGui::SameLine();
		UI::ShiftCursor(0.f, 2.f);
		ImGui::Image(UI::GetTextureID(EditorIconLibrary::GetIcon(EditorIcon::Search)), { m_buttonSize, m_buttonSize }, { 0.f, 0.f }, { 1.f, 1.f });

		ImGui::SameLine();
		UI::ShiftCursor(0.f, -2.f);
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

		UI::PushId();
		if (UI::InputText("", m_searchQuery, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			if (m_searchQuery.empty())
			{
				m_hasSearchQuery = false;
			}
			else
			{
				m_hasSearchQuery = true;
			}
		}
		UI::PopId();
		ImGui::PopItemWidth();

		UI::ScopedColor childColor(ImGuiCol_ChildBg, { 0.18f, 0.18f, 0.18f, 1.f });

		ImGui::BeginChild("meshes");
		{
			UI::ShiftCursor(5.f, 5.f);

			if (m_hasSearchQuery)
			{
			}
			else
			{
				UI::ScopedStyleFloat2 spacing(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });

				uint32_t i = 0;
				RenderDirectory(m_directories[FileSystem::GetAssetsPath().string()], i);
			}
		}
		ImGui::EndChild();
	}
}

std::shared_ptr<DirectoryData> CreatePanel::ProcessDirectory(const std::filesystem::path& path, std::shared_ptr<DirectoryData> parent)
{
	std::shared_ptr<DirectoryData> dirData = std::make_shared<DirectoryData>();
	dirData->path = path;
	dirData->parentDir = parent.get();

	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		if (!entry.is_directory())
		{
			AssetData data;
			data.path = entry;

			if (Lamp::AssetManager::Get().GetAssetTypeFromPath(data.path) == Lamp::AssetType::MeshSource)
			{
				dirData->assets.push_back(data);
			}
		}
		else
		{
			dirData->subDirectories.emplace_back(ProcessDirectory(entry, dirData));
		}
	}

	for (int32_t i = (int32_t)dirData->subDirectories.size() - 1; i >= 0; --i)
	{
		if (dirData->subDirectories[i]->assets.empty() && dirData->subDirectories[i]->subDirectories.empty())
		{
			dirData->subDirectories.erase(dirData->subDirectories.begin() + i);
		}
	}

	//Sort directories and assets by name
	std::sort(dirData->subDirectories.begin(), dirData->subDirectories.end(), [](const std::shared_ptr<DirectoryData> dataOne, const std::shared_ptr<DirectoryData> dataTwo)
		{
			return dataOne->path.stem().string() < dataTwo->path.stem().string();
		});

	std::sort(dirData->assets.begin(), dirData->assets.end(), [](const AssetData& dataOne, const AssetData& dataTwo)
		{
			return dataOne.path.stem().string() < dataTwo.path.stem().string();
		});

	return dirData;
}

void CreatePanel::RenderDirectory(const std::shared_ptr<DirectoryData> dirData, uint32_t& i)
{
	std::string id = dirData->path.stem().string() + "##" + std::to_string(i++);

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
	if (i == 0)
	{
		flags = (dirData->selected ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None) | ImGuiTreeNodeFlags_DefaultOpen;
	}
	else
	{
		flags = (dirData->selected ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None);
	}

	bool open = UI::TreeNodeImage(EditorIconLibrary::GetIcon(EditorIcon::Directory), id.c_str(), flags);

	if (open)
	{
		for (const auto& subDir : dirData->subDirectories)
		{
			RenderDirectory(subDir, i);
		}

		uint32_t j = 0;
		for (const auto& asset : dirData->assets)
		{
			std::string assetId = asset.path.stem().string() + "##" + std::to_string(j);

			UI::ImageSelectable(EditorIconLibrary::GetIcon(EditorIcon::GenericFile), assetId);

			if (ImGui::BeginDragDropSource())
			{
				const wchar_t* itemPath = asset.path.c_str();
				ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t), ImGuiCond_Once);
				ImGui::EndDragDropSource();
			}
			j++;
		}

		ImGui::TreePop();
	}
}
