#pragma once

#include "Sandbox/Window/EditorWindow.h"
#include "Sandbox/Window/SelectiveAssetBrowserPanel.h"

#include "Sandbox/NodeEditor/Graph.h"
#include "Sandbox/NodeEditor/IconDrawing.h"

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
	void SetupStyle();

	ImColor GetIconColor(PinType type);
	void DrawPinIcon(const Pin& pin, bool connected, int32_t alpha);
	void DrawIcon(ImDrawList* drawList, const ImVec2& a, const ImVec2& b, IconType type, bool filled, ImU32 color, ImU32 innerColor);

	Pin* FindPin(ax::NodeEditor::PinId id);

	ax::NodeEditor::EditorContext* m_editorContext = nullptr;

	Ref<Graph> m_graph;
	Ref<Lamp::RenderGraph> m_loadedRenderGraph;
	Ref<SelectiveAssetBrowserPanel> m_assetBrowser;
};