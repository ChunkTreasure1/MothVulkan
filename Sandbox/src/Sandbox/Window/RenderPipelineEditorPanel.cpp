#include "sbpch.h"
#include "RenderPipelineEditorPanel.h"

#include <Lamp/Rendering/RenderPipeline/RenderPipeline.h>
#include <Lamp/Rendering/RenderPass/RenderPassRegistry.h>

#include <Lamp/Rendering/Renderer.h>
#include <Lamp/Rendering/RenderPass/RenderPass.h>
#include <Lamp/Rendering/Shader/ShaderRegistry.h>
#include <Lamp/Rendering/Shader/Shader.h>

#include <Lamp/Utility/UIUtility.h>

#include <imgui.h>

RenderPipelineEditorPanel::RenderPipelineEditorPanel()
	: EditorWindow("Render Pipeline Editor", true)
{
	m_windowFlags = ImGuiWindowFlags_MenuBar;
	
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
				m_currentlySelectedShader = 0;
				m_currentlySelectedRenderPass = 0;

				const std::filesystem::path path = FileSystem::OpenFile("Render Pipeline (*.lprpdef)\0*.lprpdef\0");
				if (!path.empty())
				{
					Save();

					Ref<Lamp::RenderPipeline> newPipeline = Lamp::AssetManager::GetAsset<Lamp::RenderPipeline>(path);
					if (newPipeline)
					{
						m_loadedRenderPipeline = newPipeline;
						UpdateCurrentShaderAndRenderPass();
					}
					else
					{
						std::string content = std::format("Unable to open file: {0}!", path.string());
						UI::Notify(NotificationType::Error, "Error!", content);
					}
				}
			}

			if (ImGui::MenuItem("New", "Ctrl + N"))
			{
				m_currentlySelectedShader = 0;
				Save();

				m_loadedRenderPipeline = Lamp::RenderPipeline::Create();
			}

			if (ImGui::MenuItem("Save", "Ctrl + S"))
			{
				Save();
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

				if (UI::ComboProperty("Shader", m_currentlySelectedShader, items) && m_currentlySelectedShader > 0)
				{
					m_loadedRenderPipeline->SetShader(Lamp::ShaderRegistry::Get(items[m_currentlySelectedShader]));
				}
			}

			// Render pass
			{
				std::vector<std::string> items;
				items.emplace_back("None");
				for (const auto& [name, shader] : Lamp::RenderPassRegistry::GetAllPasses())
				{
					items.emplace_back(name);
				}

				if (UI::ComboProperty("Render pass", m_currentlySelectedRenderPass, items) && m_currentlySelectedRenderPass > 0)
				{
					m_loadedRenderPipeline->SetRenderPass(Lamp::RenderPassRegistry::Get(items[m_currentlySelectedRenderPass]));
				}
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

			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.f);
			ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.2f, 0.2f, 1.f });
			if (ImGui::Button("Add"))
			{
				vertexLayout.GetElements().insert(vertexLayout.GetElements().begin(), { Lamp::ElementType::Int, "New" });
			}
			ImGui::PopStyleVar();

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

				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.f);
				std::string buttId = "-##rem" + std::to_string(UI::GetId());
				if (ImGui::Button(buttId.c_str(), { buttonSize, buttonSize }))
				{
					vertexLayout.GetElements().erase(vertexLayout.GetElements().begin() + i);
				}
				ImGui::PopStyleVar();

			}
			UI::PopId();
			ImGui::PopStyleColor();
		}

		if (ImGui::CollapsingHeader("Instance Layout"))
		{
			auto& vertexLayout = specification.instanceLayout;

			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.f);
			ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.2f, 0.2f, 1.f });
			if (ImGui::Button("Add"))
			{
				vertexLayout.GetElements().insert(vertexLayout.GetElements().begin(), { Lamp::ElementType::Int, "New" });
			}
			ImGui::PopStyleVar();

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

				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.f );
				std::string buttId = "-##rem" + std::to_string(UI::GetId());
				if (ImGui::Button(buttId.c_str()))
				{
					vertexLayout.GetElements().erase(vertexLayout.GetElements().begin() + i);
				}
				ImGui::PopStyleVar();

				UI::PopId();
			}
			ImGui::PopStyleColor();
		}
	}
	ImGui::End();
}

void RenderPipelineEditorPanel::SaveAs()
{
	std::filesystem::path path = FileSystem::SaveFile("Render Pipeline (*.lprpdef)\0*.lprpdef\0");
	if (!path.empty())
	{
		if (path.extension().string() != ".lprpdef")
		{
			path += ".lprpdef";
		}

		Ref<Lamp::RenderPipeline> newPipeline = CreateRef<Lamp::RenderPipeline>(*m_loadedRenderPipeline);
		m_loadedRenderPipeline = newPipeline;

		m_loadedRenderPipeline->path = path;
		Lamp::AssetManager::Get().SaveAsset(m_loadedRenderPipeline);

		const std::string content = "Saved render pipeline \"" + m_loadedRenderPipeline->GetSpecification().name + "\" to: " + FileSystem::GetPathRelativeToBaseFolder(m_loadedRenderPipeline->path).string();
		UI::Notify(NotificationType::Success, "Saved!", content);

		InvalidateLoadedPipeline();
	}
}

void RenderPipelineEditorPanel::Save()
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

			const std::string content = "Saved render pipeline \"" + m_loadedRenderPipeline->GetSpecification().name + "\" to: " + FileSystem::GetPathRelativeToBaseFolder(m_loadedRenderPipeline->path).string();
			UI::Notify(NotificationType::Success, "Saved!", content);

			if (m_loadedRenderPipeline->GetSpecification().shader && m_loadedRenderPipeline->GetSpecification().framebuffer)
			{
				InvalidateLoadedPipeline();
			}
		}
	}
}

void RenderPipelineEditorPanel::InvalidateLoadedPipeline()
{
	Lamp::Renderer::SubmitInvalidation([pipeline = m_loadedRenderPipeline]()
	{
		pipeline->Invalidate();
	});
}

void RenderPipelineEditorPanel::UpdateCurrentShaderAndRenderPass()
{
	// Shader
	{
		uint32_t i = 0;
		for (const auto& [name, shader] : Lamp::ShaderRegistry::GetAllShaders())
		{
			i++;
			if (m_loadedRenderPipeline->GetSpecification().shader == shader)
			{
				m_currentlySelectedShader = i;
			}
		}
	}

	// Render pass
	{
		uint32_t i = 0;
		for (const auto& [name, renderPass] : Lamp::RenderPassRegistry::GetAllPasses())
		{
			i++;
			if (m_loadedRenderPipeline->GetSpecification().framebuffer == renderPass->framebuffer)
			{
				m_currentlySelectedRenderPass = i;
			}
		}
	}
}
