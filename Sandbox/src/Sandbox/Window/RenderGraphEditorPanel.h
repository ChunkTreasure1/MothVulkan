#pragma once

#include "Sandbox/Window/EditorWindow.h"
#include "Sandbox/Window/SelectiveAssetBrowserPanel.h"

#include "Sandbox/NodeEditor/Graph.h"

#include <imgui_node_editor.h>

namespace Lamp
{
	class RenderGraph;
}

class RenderGraphEditorPanel : public EditorWindow
{
public:
	RenderGraphEditorPanel();
	~RenderGraphEditorPanel() override;

	void UpdateMainContent() override;
	void UpdateContent() override;

private:
	void UpdateEditor();
	void LoadGraph();

	ax::NodeEditor::EditorContext* m_editorContext = nullptr;

	Ref<Graph> m_graph;
	Ref<Lamp::RenderGraph> m_loadedRenderGraph;
	Ref<SelectiveAssetBrowserPanel> m_assetBrowser;
};