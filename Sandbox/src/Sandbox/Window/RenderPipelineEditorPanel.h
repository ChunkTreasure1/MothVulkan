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
	void SaveAs();

	Ref<Lamp::RenderPipeline> m_loadedRenderPipeline;
	Ref<SelectiveAssetBrowserPanel> m_assetBrowser;
};