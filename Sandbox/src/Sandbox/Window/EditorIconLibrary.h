#pragma once

#include <Lamp/Core/Base.h>

#include <unordered_map>

namespace Lamp
{
	class Texture2D;
}

enum class EditorIcon
{
	Directory,
	Reload,
	Back,
	Settings,
	Search,
	Play,
	Stop,
	GenericFile,

	Locked,
	Unlocked,
	Visible,
	Hidden
};

class EditorIconLibrary
{
public:
	static void Initialize();
	static void Shutdown();

	static Ref<Lamp::Texture2D> GetIcon(EditorIcon icon);

private:
	static void TryLoadIcon(EditorIcon icon, const std::filesystem::path& path);

	EditorIconLibrary() = delete;

	inline static std::unordered_map<EditorIcon, Ref<Lamp::Texture2D>> s_icons;
 };