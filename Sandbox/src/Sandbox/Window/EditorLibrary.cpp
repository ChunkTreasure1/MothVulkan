#include "sbpch.h"
#include "EditorLibrary.h"

#include "Sandbox/Window/EditorWindow.h"

#include <Lamp/Log/Log.h>

void EditorLibrary::Register(Lamp::AssetType type, Ref<EditorWindow> editor)
{
	s_editors.emplace(type, editor);
}

void EditorLibrary::OpenAsset(Ref<Lamp::Asset> asset)
{
	if (!asset)
	{
		return;
	}

	const Lamp::AssetType type = asset->GetType();
	auto it = s_editors.find(type);
	if (it == s_editors.end())
	{
		LP_ERROR("Editor for asset not registered!");
		return;
	}

	it->second->Open(asset);
}

Ref<EditorWindow> EditorLibrary::Get(Lamp::AssetType type)
{
	auto it = s_editors.find(type);
	if (it == s_editors.end())
	{
		LP_ERROR("Editor for asset not registered!");
		return nullptr;
	}

	return it->second;
}
