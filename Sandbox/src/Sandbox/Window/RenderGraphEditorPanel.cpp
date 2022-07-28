#include "sbpch.h"
#include "RenderGraphEditorPanel.h"

#include "Sandbox/NodeEditor/IconDrawing.h"

#include <Lamp/Asset/AssetManager.h>

#include <Lamp/Rendering/RenderGraph.h>
#include <Lamp/Rendering/RenderPass/RenderPass.h>

#include <imgui_internal.h>

namespace ed = ax::NodeEditor;

static int32_t s_nodeId = 0;

RenderGraphEditorPanel::RenderGraphEditorPanel()
	: EditorWindow("Render Graph Editor", true)
{
	m_windowFlags = ImGuiWindowFlags_MenuBar;

	m_assetBrowser = CreateRef<SelectiveAssetBrowserPanel>(Lamp::AssetType::RenderGraph, "renderGraphPanel");
	m_assetBrowser->SetOpenFileCallback([this](Lamp::AssetHandle asset)
		{
			m_loadedRenderGraph = Lamp::AssetManager::GetAsset<Lamp::RenderGraph>(asset);
			LoadGraph();
		});

	m_editorContext = ed::CreateEditor();
}

RenderGraphEditorPanel::~RenderGraphEditorPanel()
{
	ed::DestroyEditor(m_editorContext);
}

void RenderGraphEditorPanel::UpdateMainContent()
{
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
	{
		ed::SetCurrentEditor(m_editorContext);
	}
}

void RenderGraphEditorPanel::UpdateContent()
{
	UpdateEditor();

	if (m_assetBrowser->Begin())
	{
		m_assetBrowser->UpdateMainContent();
		m_assetBrowser->End();
	}
}

void RenderGraphEditorPanel::UpdateEditor()
{
	ImGui::Begin("Editor##renderGraph");
	ed::Begin("renderGraphEditor");

	if (m_graph)
	{
		for (const auto& node : m_graph->nodes)
		{
			ImVec2 headerMin, headerMax;

			ed::PushStyleVar(ed::StyleVar::StyleVar_NodePadding, ImVec4(8, 4, 8, 8));
			ed::BeginNode(node.id);

			// Draw header
			{
				ImGui::Text(node.name.c_str());

				headerMax = ImGui::GetItemRectMax();
				headerMin = ImGui::GetItemRectMin();
			}

			// Content
			{
				auto startPos = ImGui::GetCursorPos();

				for (const auto& input : node.inputs)
				{
					ed::BeginPin(input.id, ax::NodeEditor::PinKind::Input);
					ImGui::TextUnformatted("<-");
					ed::EndPin();
				}

				ImGui::SetCursorPos(startPos + ImVec2{ 30.f, 0.f });

				for (const auto& output : node.outputs)
				{
					ed::BeginPin(output.id, ax::NodeEditor::PinKind::Output);
					ImGui::TextUnformatted("->");
					ed::EndPin();
				}
			}

			ed::EndNode();

			// Draw header background
			{
				if (ImGui::IsItemVisible())
				{
					headerMax.x = ImGui::GetItemRectMax().x;

					auto drawList = ed::GetNodeBackgroundDrawList(node.id);

					const auto halfBorderWidth = ed::GetStyle().NodeBorderWidth * 0.5f;

					if ((headerMax.x > headerMin.x) && (headerMax.y > headerMin.y))
					{
						//drawList->AddRectFilled(headerMin - ImVec2(8 - halfBorderWidth, 4 - halfBorderWidth),
						//						headerMax + ImVec2(8 - halfBorderWidth, 0), IM_COL32(255, 0, 255, 255), );

						drawList->AddRectFilled(headerMin - ImVec2(halfBorderWidth, 0.f),
												headerMax, IM_COL32(255, 0, 255, 255));
					}
				}
			}

			ed::PopStyleVar();
		}
	}

	ed::End();
	ImGui::End();
}

void RenderGraphEditorPanel::LoadGraph()
{
	m_graph = nullptr;
	s_nodeId = 1;

	m_graph = CreateRef<Graph>();

	//Add default nodes
	{
		auto& beginNode = m_graph->nodes.emplace_back(s_nodeId++, "Begin");
		beginNode.outputs.emplace_back(s_nodeId++, "->O", PinType::Flow, PinMode::Output);

		auto& endNode = m_graph->nodes.emplace_back(s_nodeId++, "End");
		endNode.inputs.emplace_back(s_nodeId++, "->I", PinType::Flow, PinMode::Input);
	}

	std::vector<Lamp::RenderGraph::RenderPassContainer> renderPasses = m_loadedRenderGraph->GetRenderPasses();

	for (const auto& pass : renderPasses)
	{
		auto& node = m_graph->nodes.emplace_back(s_nodeId++, pass.renderPass->name);
		node.inputs.emplace_back(s_nodeId++, "->I", PinType::Flow, PinMode::Input);
		node.outputs.emplace_back(s_nodeId++, "->O", PinType::Flow, PinMode::Output);
	}
}
