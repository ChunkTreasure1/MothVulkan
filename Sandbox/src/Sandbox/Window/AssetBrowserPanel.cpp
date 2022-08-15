#include "sbpch.h"
#include "AssetBrowserPanel.h"

#include "Sandbox/Window/EditorIconLibrary.h"
#include "Sandbox/Window/EditorLibrary.h"

#include <Lamp/Asset/AssetManager.h>
#include <Lamp/Rendering/Shader/Shader.h>

#include <Lamp/Utility/FileSystem.h>
#include <Lamp/Utility/UIUtility.h>

AssetBrowserPanel::AssetBrowserPanel()
	: EditorWindow("Asset Browser"), m_currentDirectoryPath(FileSystem::GetAssetsPath())
{
	m_windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	m_isOpen = true;

	m_directories[FileSystem::GetAssetsPath().string()] = ProcessDirectory(FileSystem::GetAssetsPath().string(), nullptr);
	m_directories[FileSystem::GetEnginePath().string()] = ProcessDirectory(FileSystem::GetEnginePath().string(), nullptr);

	m_engineDirectory = m_directories[FileSystem::GetEnginePath().string()].get();
	m_assetsDirectory = m_directories[FileSystem::GetAssetsPath().string()].get();

	m_assetIcons[Lamp::AssetType::Material] = Lamp::AssetManager::GetAsset<Lamp::Texture2D>("Editor/Textures/Icons/AssetIcons/icon_material.dds");
	m_assetIcons[Lamp::AssetType::Mesh] = Lamp::AssetManager::GetAsset<Lamp::Texture2D>("Editor/Textures/Icons/AssetIcons/icon_mesh.dds");
	m_assetIcons[Lamp::AssetType::MeshSource] = Lamp::AssetManager::GetAsset<Lamp::Texture2D>("Editor/Textures/Icons/AssetIcons/icon_meshSource.dds");

	m_currentDirectory = m_assetsDirectory;

	m_directoryButtons.emplace_back(m_currentDirectory);
}

void AssetBrowserPanel::UpdateMainContent()
{
	float cellSize = m_thumbnailSize + m_thumbnailPadding;

	if (m_nextDirectory)
	{
		m_currentDirectory->selected = false;
		m_currentDirectory = m_nextDirectory;
		m_nextDirectory = nullptr;

		m_directoryButtons.clear();
		m_directoryButtons = FindParentDirectoriesOfDirectory(m_currentDirectory);
	}

	UI::PushId();
	const float controlsBarHeight = 30.f;

	// Controls bar
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
		RenderControlsBar(controlsBarHeight);
		ImGui::PopStyleVar();
	}

	const ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable;
	if (ImGui::BeginTable("assetBrowserMain", 2, tableFlags))
	{
		ImGui::TableSetupColumn("Outline", 0, 250.f);
		ImGui::TableSetupColumn("View", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		//Draw outline
		{
			ImGuiStyle& style = ImGui::GetStyle();
			auto color = style.Colors[ImGuiCol_FrameBg];

			UI::ScopedColor newColor(ImGuiCol_ChildBg, { color.x, color.y, color.z, color.w });
			ImGui::BeginChild("##outline");
		}

		{
			UI::ShiftCursor(5.f, 5.f);
			auto flags = (m_assetsDirectory ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;

			bool open = UI::TreeNodeImage(EditorIconLibrary::GetIcon(EditorIcon::Directory), "Assets", flags);

			if (ImGui::IsItemClicked())
			{
				m_assetsDirectory->selected = true;
				m_nextDirectory = m_assetsDirectory;
			}

			if (open)
			{
				UI::ScopedStyleFloat2 spacing(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });

				for (const auto& subDir : m_assetsDirectory->subDirectories)
				{
					RenderDirectory(subDir);
				}
				UI::TreeNodePop();
			}
		}
		ImGui::EndChild();

		ImGui::TableNextColumn();

		ImGui::BeginChild("##view", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetWindowHeight() - controlsBarHeight));
		{
			ImGui::BeginChild("Scrolling");
			{
				static float padding = 16.f;

				float cellSize = m_thumbnailSize + padding;
				float panelWidth = ImGui::GetContentRegionAvail().x;
				int columnCount = (int)(panelWidth / cellSize);

				if (columnCount < 1)
				{
					columnCount = 1;
				}

				ImGui::Columns(columnCount, nullptr, false);
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.f, 0.f, 0.f, 0.f });

				if (!m_hasSearchQuery)
				{
					RenderView(m_currentDirectory->subDirectories, m_currentDirectory->assets);
				}
				else
				{
					RenderView(m_searchDirectories, m_searchAssets);
				}

				ImGui::PopStyleColor();
			}
			ImGui::EndChild();
		}
		ImGui::EndChild();

		ImGui::EndTable();
	}

	UI::PopId();
}

Ref<DirectoryData> AssetBrowserPanel::ProcessDirectory(const std::filesystem::path& path, Ref<DirectoryData> parent)
{
	Ref<DirectoryData> dirData = CreateRef<DirectoryData>();
	dirData->path = path;
	dirData->parentDir = parent.get();

	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		if (!entry.is_directory())
		{
			AssetData assetData;
			assetData.path = entry;
			assetData.handle = Lamp::AssetManager::Get().GetAssetHandleFromPath(entry);
			assetData.type = Lamp::AssetManager::Get().GetAssetTypeFromPath(entry);

			if (assetData.type != Lamp::AssetType::None)
			{
				dirData->assets.emplace_back(assetData);
			}
		}
		else
		{
			dirData->subDirectories.emplace_back(ProcessDirectory(entry, dirData));
		}
	}

	std::sort(dirData->subDirectories.begin(), dirData->subDirectories.end(), [](const Ref<DirectoryData>& a, const Ref<DirectoryData>& b) { return a->path.string() < b->path.string(); });
	std::sort(dirData->assets.begin(), dirData->assets.end(), [](const AssetData& a, const AssetData& b) { return a.path.stem().string() < b.path.stem().string(); });

	return dirData;
}

std::vector<DirectoryData*> AssetBrowserPanel::FindParentDirectoriesOfDirectory(DirectoryData* directory)
{
	std::vector<DirectoryData*> directories;
	directories.emplace_back(directory);

	for (auto dir = directory->parentDir; dir != nullptr; dir = dir->parentDir)
	{
		directories.emplace_back(dir);
	}

	std::reverse(directories.begin(), directories.end());
	return directories;
}

void AssetBrowserPanel::RenderControlsBar(float height)
{
	UI::ScopedColor childColor{ ImGuiCol_ChildBg, { 0.2f, 0.2f, 0.2f, 1.f } };

	ImGui::BeginChild("##controlsBar", { 0.f, height });
	{
		const float buttonSizeOffset = 10.f;
		int32_t offsetToRemove = 0;
		bool shouldRemove = false;

		UI::ShiftCursor(5.f, 4.f);
		{
			UI::ScopedColor buttonBackground(ImGuiCol_Button, { 0.f, 0.f, 0.f, 0.f });
			ImGui::Image(UI::GetTextureID(EditorIconLibrary::GetIcon(EditorIcon::Search)), { height - buttonSizeOffset, height - buttonSizeOffset });

			ImGui::SameLine();
			UI::ShiftCursor(0.f, -0.5f);
			ImGui::PushItemWidth(200.f);

			if (UI::InputText("", m_searchQuery, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				if (!m_searchQuery.empty())
				{
					m_hasSearchQuery = true;
					Search(m_searchQuery);
				}
				else
				{
					m_hasSearchQuery = false;
				}
			}

			ImGui::PopItemWidth();
			ImGui::SameLine();
			UI::ShiftCursor(0.f, -1.f);
			{
				UI::ScopedColor buttonBackground(ImGuiCol_Button, { 0.f, 0.f, 0.f, 0.f });

				if (UI::ImageButton("##reloadButton", UI::GetTextureID(EditorIconLibrary::GetIcon(EditorIcon::Reload)), { height - buttonSizeOffset, height - buttonSizeOffset }))
				{
					Reload();
				}

				ImGui::SameLine();

				if (UI::ImageButton("##backButton", UI::GetTextureID(EditorIconLibrary::GetIcon(EditorIcon::Back)), { height - buttonSizeOffset, height - buttonSizeOffset }))
				{
					if (m_currentDirectory->path != FileSystem::GetAssetsPath())
					{
						m_nextDirectory = m_currentDirectory->parentDir;
						m_nextDirectory->selected = true;

						offsetToRemove = (uint32_t)(m_directoryButtons.size() - 1);
						shouldRemove = true;
					}
				}
			}

			for (size_t i = 0; i < m_directoryButtons.size(); i++)
			{
				ImGui::SameLine();
				std::string dirName = m_directoryButtons[i]->path.stem().string();

				const float buttonWidth = ImGui::CalcTextSize(dirName.c_str()).x + 10.f;

				if (ImGui::Button(dirName.c_str(), { buttonWidth, height - 4.f }))
				{
					m_nextDirectory = m_directoryButtons[i];
					m_nextDirectory->selected = true;

					offsetToRemove = (int32_t)(i + 1);
					shouldRemove = true;
				}
			}

			ImGui::SameLine();

			// Asset type
			{
				const char* items = "Game\0Engine";
				
				int32_t currentValue = (int32_t)m_showEngineAssets;

				UI::ShiftCursor(ImGui::GetContentRegionAvail().x - height - 150.f - buttonSizeOffset / 2.f, 0.f);

				ImGui::PushItemWidth(150.f);
				if (ImGui::Combo("##assetType", &currentValue, items))
				{
					m_showEngineAssets = (bool)currentValue;
					if (currentValue == 0)
					{
						m_currentDirectory = m_assetsDirectory;
					}
					else
					{
						m_currentDirectory = m_engineDirectory;
					}

				}
				ImGui::PopItemWidth();
			} 

			ImGui::SameLine();

			// Settings button
			{
				UI::ScopedColor buttonBackground(ImGuiCol_Button, { 0.f, 0.f, 0.f, 0.f });

				if (ImGui::ImageButton(UI::GetTextureID(EditorIconLibrary::GetIcon(EditorIcon::Settings)), { height - buttonSizeOffset, height - buttonSizeOffset }))
				{
				}
			}

			if (ImGui::BeginPopupContextItem("settingsMenu", ImGuiPopupFlags_MouseButtonLeft))
			{
				ImGui::PushItemWidth(100.f);
				ImGui::SliderFloat("Icon size", &m_thumbnailSize, 20.f, 200.f);
				ImGui::PopItemWidth();

				ImGui::EndPopup();
			}

			if (shouldRemove)
			{
				for (int32_t i = (int32_t)m_directoryButtons.size() - 1; i >= offsetToRemove; i--)
				{
					m_directoryButtons.erase(m_directoryButtons.begin() + i);
				}
			}
		}
	}
	ImGui::EndChild();
}

void AssetBrowserPanel::RenderDirectory(const Ref<DirectoryData> dirData)
{
	std::string id = dirData->path.stem().string() + "##" + std::to_string(dirData->handle);

	auto flags = (dirData->selected ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None) | ImGuiTreeNodeFlags_OpenOnArrow;

	bool open = UI::TreeNodeImage(EditorIconLibrary::GetIcon(EditorIcon::Directory), id.c_str(), flags);
	if (ImGui::IsItemClicked())
	{
		dirData->selected = true;
		m_nextDirectory = dirData.get();
	}

	if (open)
	{
		for (const auto& subDir : dirData->subDirectories)
		{
			RenderDirectory(subDir);
		}

		for (const auto& asset : dirData->assets)
		{
			std::string assetId = asset.path.stem().string() + "##" + std::to_string(asset.handle);
			ImGui::Selectable(assetId.c_str());
		}

		ImGui::TreePop();
	}
}

void AssetBrowserPanel::RenderView(const std::vector<Ref<DirectoryData>>& dirData, const std::vector<AssetData>& assetData)
{
	for (auto& dir : dirData)
	{
		ImGui::PushID(dir->path.filename().string().c_str());

		UI::ImageButton(dir->path.filename().string(), UI::GetTextureID(EditorIconLibrary::GetIcon(EditorIcon::Directory)), { m_thumbnailSize, m_thumbnailSize });
		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered()) 
		{
			dir->selected = true;
			m_nextDirectory = dir.get();
		}

		ImGui::TextWrapped(dir->path.filename().string().c_str());

		ImGui::NextColumn();
		ImGui::PopID();
	}

	for (const auto& asset : assetData)
	{
		ImGui::PushID(asset.path.filename().string().c_str());

		Ref<Lamp::Texture2D> icon = m_assetIcons[asset.type];
		if (!icon)
		{
			icon = EditorIconLibrary::GetIcon(EditorIcon::GenericFile);
		}

		UI::ImageButton(asset.path.filename().string(), UI::GetTextureID(icon), { m_thumbnailSize, m_thumbnailSize });
		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered())
		{
			EditorLibrary::OpenAsset(Lamp::AssetManager::Get().GetAssetRaw(asset.handle));
		}

		if (ImGui::BeginDragDropSource())
		{
			//Data being copied
			ImGui::SetDragDropPayload("ASSET_BROWSER_ITEM", &asset.handle, sizeof(Lamp::AssetHandle), ImGuiCond_Once);
			ImGui::EndDragDropSource();
		}
		RenderFilePopup(asset);

		ImGui::TextWrapped(asset.path.filename().string().c_str());

		ImGui::NextColumn();
		ImGui::PopID();
	}
}

void AssetBrowserPanel::RenderFilePopup(const AssetData& data)
{
	if (UI::BeginPopup())
	{
		if (ImGui::MenuItem("Open Externally"))
		{
			FileSystem::OpenFileExternally(data.path);
		}

		switch (data.type)
		{
			case Lamp::AssetType::Shader:
			{
				if (ImGui::MenuItem("Recompile"))
				{
					Ref<Lamp::Shader> shader = Lamp::AssetManager::GetAsset<Lamp::Shader>(data.path);
					shader->Reload(true);
				}

				break;
			}

			default:
				break;
		}

		UI::EndPopup();
	}
}

void AssetBrowserPanel::Reload()
{
	const std::filesystem::path currentPath = m_currentDirectory ? m_currentDirectory->path : FileSystem::GetAssetsPath();

	m_currentDirectory = nullptr;
	m_nextDirectory = nullptr;

	m_directories[FileSystem::GetAssetsPath().string()] = ProcessDirectory(FileSystem::GetAssetsPath().string(), nullptr);
	m_directories[FileSystem::GetEnginePath().string()] = ProcessDirectory(FileSystem::GetEnginePath().string(), nullptr);

	m_engineDirectory = m_directories[FileSystem::GetEnginePath().string()].get();
	m_assetsDirectory = m_directories[FileSystem::GetAssetsPath().string()].get();

	//Find directory
	m_currentDirectory = FindDirectoryWithPath(currentPath);
	if (!m_currentDirectory)
	{
		m_currentDirectory = m_assetsDirectory;
	}

	//Setup new file path buttons
	m_directoryButtons.clear();
	m_directoryButtons = FindParentDirectoriesOfDirectory(m_currentDirectory);
}

void AssetBrowserPanel::Search(const std::string& query)
{
	std::vector<std::string> queries;
	std::vector<std::string> types;

	std::string searchQuery = query;

	searchQuery.push_back(' ');

	for (auto next = searchQuery.find_first_of(' '); next != std::string::npos; next = searchQuery.find_first_of(' '))
	{
		std::string split = searchQuery.substr(0, next);
		searchQuery = searchQuery.substr(next + 1);

		if (split.front() == '*')
		{
			types.emplace_back(split);
		}
		else
		{
			queries.emplace_back(split);
		}
	}

	//Find all folders and files containing queries
	m_searchDirectories.clear();
	m_searchAssets.clear();
	for (const auto& query : queries)
	{
		FindFoldersAndFilesWithQuery(m_assetsDirectory->subDirectories, m_searchDirectories, m_searchAssets, query);
	}
}

void AssetBrowserPanel::FindFoldersAndFilesWithQuery(const std::vector<Ref<DirectoryData>>& dirList, std::vector<Ref<DirectoryData>>& directories, std::vector<AssetData>& assets, const std::string& query)
{
	for (const auto& dir : dirList)
	{
		std::string dirStem = dir->path.stem().string();
		std::transform(dirStem.begin(), dirStem.end(), dirStem.begin(), [](unsigned char c)
			{
				return std::tolower(c);
			});

		if (dirStem.find(query) != std::string::npos)
		{
			directories.emplace_back(dir);
		}

		for (const auto& asset : dir->assets)
		{
			std::string assetStem = asset.path.stem().string();
			std::transform(assetStem.begin(), assetStem.end(), assetStem.begin(), [](unsigned char c)
				{
					return std::tolower(c);
				});

			if (assetStem.find(query) != std::string::npos)
			{
				assets.emplace_back(asset);
			}
		}

		FindFoldersAndFilesWithQuery(dir->subDirectories, directories, assets, query);
	}
}

DirectoryData* AssetBrowserPanel::FindDirectoryWithPath(const std::filesystem::path& path)
{
	std::vector<Ref<DirectoryData>> dirList;
	for (const auto& dir : m_directories)
	{
		dirList.emplace_back(dir.second);
	}

	return FindDirectoryWithPathRecursivly(dirList, path);
}

DirectoryData* AssetBrowserPanel::FindDirectoryWithPathRecursivly(const std::vector<Ref<DirectoryData>> dirList, const std::filesystem::path& path)
{
	for (const auto& dir : dirList)
	{
		if (dir->path == path)
		{
			return dir.get();
		}
	}

	for (const auto& dir : dirList)
	{
		if (auto it = FindDirectoryWithPathRecursivly(dir->subDirectories, path))
		{
			return it;
		}
	}

	return nullptr;
}