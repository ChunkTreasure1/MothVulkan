#include "sbpch.h"
#include "RenderPipelineEditorPanel.h"

#include <Lamp/Rendering/RenderPipeline/RenderPipeline.h>
#include <Lamp/Rendering/RenderPass/RenderPassRegistry.h>

#include <Lamp/Rendering/Shader/ShaderRegistry.h>
#include <Lamp/Rendering/Shader/Shader.h>

#include <Lamp/Utility/UIUtility.h>

#include <imgui.h>

RenderPipelineEditorPanel::RenderPipelineEditorPanel()
	: EditorWindow("Render Pipeline Editor", true)
{
	m_windowFlags = ImGuiWindowFlags_MenuBar;
	m_loadedRenderPipeline = Lamp::AssetManager::GetAsset<Lamp::RenderPipeline>("Engine/RenderPipelines/pbrPipeline.lprpdef");
	m_assetBrowser = CreateRef<SelectiveAssetBrowserPanel>(Lamp::AssetType::RenderPipeline, "renderPipelinePanel");

	m_assetBrowser->SetOpenFileCallback([this](Lamp::AssetHandle asset)
		{
			m_loadedRenderPipeline = Lamp::AssetManager::GetAsset<Lamp::RenderPipeline>(asset);
		});
}

void RenderPipelineEditorPanel::UpdateMainContent()
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open", "Ctrl + O"))
			{
			}

			if (ImGui::MenuItem("New", "Ctrl + N"))
			{
				if (m_loadedRenderPipeline && !m_loadedRenderPipeline->path.empty())
				{
					Lamp::AssetManager::Get().SaveAsset(m_loadedRenderPipeline);
				}

				m_loadedRenderPipeline = Lamp::RenderPipeline::Create();
			}

			if (ImGui::MenuItem("Save", "Ctrl + S"))
			{
				if (m_loadedRenderPipeline)
				{
					if (m_loadedRenderPipeline->path.empty())
					{
						SaveAs();
					}
					else
					{
						Lamp::AssetManager::Get().SaveAsset(m_loadedRenderPipeline);
					}
				}
			}

			if (ImGui::MenuItem("Save As", "Ctrl + Shift + S"))
			{
				SaveAs();
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
}

void RenderPipelineEditorPanel::UpdateContent()
{
	UpdateEditor();

	if (m_assetBrowser->Begin())
	{
		m_assetBrowser->UpdateMainContent();
		m_assetBrowser->End();
	}
}

void RenderPipelineEditorPanel::UpdateEditor()
{
	ImGui::Begin("Editor");
	if (m_loadedRenderPipeline)
	{
		auto& specification = const_cast<Lamp::RenderPipelineSpecification&>(m_loadedRenderPipeline->GetSpecification());

		UI::PushId();
		if (UI::BeginProperties())
		{
			UI::Property("Name", specification.name);

			// Shader
			{
				std::vector<std::string> items;
				items.emplace_back("None");
				for (const auto& [name, shader] : Lamp::ShaderRegistry::GetAllShaders())
				{
					items.emplace_back(name);

				}

				int32_t currentItem = 0;
				UI::ComboProperty("Shader", currentItem, items);
			}

			// Topology
			{
				const std::vector<const char*> items = { "TriangleList", "LineList", "TriangleStrip", "PatchList" };
				UI::ComboProperty("Topology", *(int32_t*)&specification.topology, items);
			}

			// Cull mode
			{
				const std::vector<const char*> items = { "Front", "Back", "FrontAndBack", "None" };
				UI::ComboProperty("Cull Mode", *(int32_t*)&specification.cullMode, items);
			}

			// Fill mode
			{
				const std::vector<const char*> items = { "Solid", "Wireframe" };
				UI::ComboProperty("Fill Mode", *(int32_t*)&specification.fillMode, items);
			}

			// Cull mode
			{
				const std::vector<const char*> items = { "Read", "Write", "ReadWrite", "None" };
				UI::ComboProperty("Depth Mode", *(int32_t*)&specification.depthMode, items);
			}

			UI::Property("Depth Test", specification.depthTest);
			UI::Property("Depth Write", specification.depthWrite);
			UI::Property("Line Width", specification.lineWidth);
			UI::Property("Tessellation Control Points", specification.tessellationControlPoints);

			UI::EndProperties();
		}
		UI::PopId();

		if (ImGui::CollapsingHeader("Vertex Layout"))
		{
			auto& vertexLayout = specification.vertexLayout;

			UI::PushId();
			for (int32_t i = (int32_t)vertexLayout.GetElements().size() - 1; i >= 0; i--)
			{
				auto& element = vertexLayout.GetElements()[i];

				constexpr float buttonSize = 22.f;

				ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x / 2.f - buttonSize);
				std::string nameId = "##name" + std::to_string(UI::GetId());
				ImGui::InputTextString(nameId.c_str(), &element.name);

				ImGui::SameLine();

				// Type
				{
					std::vector<const char*> types = { "Bool", "Int", "UInt", "UInt2", "UInt3", "UInt4", "Float", "Float2", "Float3", "Float4", "Mat3", "Mat4" };
					std::string typeId = "##type" + std::to_string(UI::GetId());
					ImGui::Combo(typeId.c_str(), (int32_t*)&element.type, types.data(), (int32_t)types.size());
				}

				ImGui::PopItemWidth();

				ImGui::SameLine();

				std::string buttId = "-##rem" + std::to_string(UI::GetId());
				if (ImGui::Button(buttId.c_str(), { buttonSize, buttonSize }))
				{
					vertexLayout.GetElements().erase(vertexLayout.GetElements().begin() + i);
				}

			}
			UI::PopId();
		}

		if (ImGui::CollapsingHeader("Instance Layout"))
		{
			auto& vertexLayout = specification.instanceLayout;

			for (int32_t i = (int32_t)vertexLayout.GetElements().size() - 1; i >= 0; i--)
			{
				auto& element = vertexLayout.GetElements()[i];

				UI::PushId();

				std::string nameId = "##name" + std::to_string(UI::GetId());

				ImGui::InputTextString(nameId.c_str(), &element.name);
				ImGui::SameLine();

				// Type
				{
					std::vector<const char*> types = { "Bool", "Int", "UInt", "UInt2", "UInt3", "UInt4", "Float", "Float2", "Float3", "Float4", "Mat3", "Mat4" };
					std::string typeId = "##type" + std::to_string(UI::GetId());
					ImGui::Combo(typeId.c_str(), (int32_t*)&element.type, types.data(), (int32_t)types.size());
				}

				ImGui::SameLine();

				std::string buttId = "-##rem" + std::to_string(UI::GetId());
				if (ImGui::Button(buttId.c_str()))
				{
					vertexLayout.GetElements().erase(vertexLayout.GetElements().begin() + i);
				}

				UI::PopId();
			}
		}
	}
	ImGui::End();
}

void RenderPipelineEditorPanel::SaveAs()
{
	std::filesystem::path path = FileSystem::SaveFile("Render Pipeline (*.lprpdef)\0*.lprpdef\0");
	if (!path.empty())
	{
		if (!m_loadedRenderPipeline->path.empty())
		{
			Lamp::AssetManager::Get().SaveAsset(m_loadedRenderPipeline);
			FileSystem::Copy(m_loadedRenderPipeline->path, path);
		
			m_loadedRenderPipeline = Lamp::AssetManager::Get().GetAsset<Lamp::RenderPipeline>(path);
		}
		else
		{
			m_loadedRenderPipeline->path = path;
			Lamp::AssetManager::Get().SaveAsset(m_loadedRenderPipeline);
		}
	}
}