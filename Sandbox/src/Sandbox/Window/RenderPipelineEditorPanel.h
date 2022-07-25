#pragma once

#include "Sandbox/Window/EditorWindow.h"
#include "Sandbox/Window/SelectiveAssetBrowserPanel.h"

namespace Lamp
{
	class RenderPipeline;
}

class RenderPipelineEditorPanel : public EditorWindow
{
public:
	RenderPipelineEditorPanel();

	void UpdateMainContent() override;
	void UpdateContent() override;

private:
	void UpdateEditor();
	void InvalidateLoadedPipeline();
	void UpdateCurrentShaderAndRenderPass();

	void SaveAs();
	void Save();

	Ref<Lamp::RenderPipeline> m_loadedRenderPipeline;
	Ref<SelectiveAssetBrowserPanel> m_assetBrowser;

	int32_t m_currentlySelectedShader = 0;
	int32_t m_currentlySelectedRenderPass = 0;

};