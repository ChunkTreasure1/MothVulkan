#include "sbpch.h"
#include "EditorIconLibrary.h"

#include <Lamp/Asset/AssetManager.h>
#include <Lamp/Rendering/Renderer.h>

void EditorIconLibrary::Initialize()
{
	TryLoadIcon(EditorIcon::Directory, "Editor/Textures/Icons/icon_directory.dds");
	TryLoadIcon(EditorIcon::Back, "Editor/Textures/Icons/icon_back.dds");
	TryLoadIcon(EditorIcon::Reload, "Editor/Textures/Icons/icon_reload.dds");
	TryLoadIcon(EditorIcon::Search, "Editor/Textures/Icons/icon_search.dds");
	TryLoadIcon(EditorIcon::Settings, "Editor/Textures/Icons/icon_settings.dds");
	TryLoadIcon(EditorIcon::Play, "Editor/Textures/Icons/icon_play.dds");
	TryLoadIcon(EditorIcon::Stop, "Editor/Textures/Icons/icon_stop.dds");
	TryLoadIcon(EditorIcon::GenericFile, "Editor/Textures/Icons/icon_file.dds");      

	TryLoadIcon(EditorIcon::Unlocked, "Editor/Textures/Icons/icon_unlocked.dds");
	TryLoadIcon(EditorIcon::Locked, "Editor/Textures/Icons/icon_locked.dds");

	TryLoadIcon(EditorIcon::Hidden, "Editor/Textures/Icons/icon_hidden.dds");
	TryLoadIcon(EditorIcon::Visible, "Editor/Textures/Icons/icon_visible.dds");
}

void EditorIconLibrary::Shutdown()
{
	s_icons.clear();
}

Ref<Lamp::Texture2D> EditorIconLibrary::GetIcon(EditorIcon icon)
{
	if (s_icons.find(icon) != s_icons.end())
	{
		return s_icons[icon];
	}

	LP_CORE_ASSERT(false);
	return nullptr;
}

void EditorIconLibrary::TryLoadIcon(EditorIcon icon, const std::filesystem::path& path)
{
	Ref<Lamp::Texture2D> texture = Lamp::AssetManager::GetAsset<Lamp::Texture2D>(path);
	if (!texture)
	{
		texture = Lamp::Renderer::GetDefaultData().whiteTexture;
	}

	s_icons[icon] = texture;
}
