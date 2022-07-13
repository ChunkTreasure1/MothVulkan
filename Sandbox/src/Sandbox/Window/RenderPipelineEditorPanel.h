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
	Ref<Lamp::RenderPipeline> m_loadedRenderPipeline;
	Ref<SelectiveAssetBrowserPanel> m_assetBrowser;
};