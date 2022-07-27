#pragma once

#include "Sandbox/Window/EditorWindow.h"
#include "Sandbox/Window/SelectiveAssetBrowserPanel.h"

namespace Lamp
{
	class RenderPass;
}

class RenderPassEditorPanel : public EditorWindow
{
public:
	RenderPassEditorPanel();

	void UpdateMainContent() override;
	void UpdateContent() override;

private:
	void UpdateEditor();

	void Save();
	void SaveAs();

	int32_t m_currentOverridePipeline = 0;

	Ref<Lamp::RenderPass> m_loadedRenderPass;
	Ref<SelectiveAssetBrowserPanel> m_assetBrowser;
};