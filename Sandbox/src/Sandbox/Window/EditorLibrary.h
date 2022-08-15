#pragma once

#include <Lamp/Asset/Asset.h>
#include <unordered_map>

class EditorWindow;

class EditorLibrary
{
public:
	static void Register(Lamp::AssetType type, Ref<EditorWindow> editor);
	static void OpenAsset(Ref<Lamp::Asset> asset);
	static Ref<EditorWindow> Get(Lamp::AssetType type);

private:
	inline static std::unordered_map<Lamp::AssetType, Ref<EditorWindow>> s_editors;
};