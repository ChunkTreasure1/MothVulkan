#pragma once

#include "Sandbox/Window/EditorWindow.h"
#include "Sandbox/Window/AssetCommon.h"

class SelectiveAssetBrowserPanel : public EditorWindow
{
public:
	SelectiveAssetBrowserPanel(Lamp::AssetType assetType, const std::string& id);

	void UpdateMainContent() override;
	inline void SetOpenFileCallback(std::function<void(Lamp::AssetHandle asset)>&& function) { m_openFileCallback = function; }

private:
	void UpdateAssetList();
	void RenderControlsBar();

	Lamp::AssetType m_selectiveAssetType;

	std::vector<AssetData> m_allAssetsOfType;
	std::function<void(Lamp::AssetHandle asset)> m_openFileCallback;

	float m_thumbnailSize = 70.f;
	float m_thumbnailPadding = 16.f;
	const float m_controlsBarHeight = 30.f;
};