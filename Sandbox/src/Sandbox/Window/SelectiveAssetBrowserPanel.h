#pragma once

#include "Sandbox/Window/EditorWindow.h"
#include "Sandbox/Window/AssetCommon.h"

class SelectiveAssetBrowserPanel : public EditorWindow
{
public:
	SelectiveAssetBrowserPanel(Lamp::AssetType assetType);

	void UpdateMainContent() override;

private:
	void UpdateAssetList();
	void RenderControlsBar();

	Lamp::AssetType m_selectiveAssetType;
	std::vector<AssetData> m_allAssetsOfType;

	float m_thumbnailSize = 70.f;
	float m_thumbnailPadding = 16.f;
	const float m_controlsBarHeight = 30.f;
};