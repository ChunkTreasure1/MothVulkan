#include "sbpch.h"
#include "RenderPassEditorPanel.h"

#include <Lamp/Asset/AssetManager.h>
#include <Lamp/Asset/RenderPipelineAsset.h>

#include <Lamp/Rendering/RenderPipeline/RenderPipelineRegistry.h>

#include <Lamp/Rendering/RenderPass/RenderPass.h>
#include <Lamp/Rendering/RenderPass/RenderPassRegistry.h>
#include <Lamp/Rendering/Framebuffer.h>

#include <Lamp/Utility/UIUtility.h>

RenderPassEditorPanel::RenderPassEditorPanel()
	: EditorWindow("Render Pass Editor", true)
{
	m_windowFlags = ImGuiWindowFlags_MenuBar;

	{
		m_loadedRenderPass = Lamp::AssetManager::GetAsset<Lamp::RenderPass>("Engine/RenderPasses/forward.lprp");
	}

	m_assetBrowser = CreateRef<SelectiveAssetBrowserPanel>(Lamp::AssetType::RenderPass, "renderPassPanel");
	m_assetBrowser->SetOpenFileCallback([this](Lamp::AssetHandle asset)
		{
			Save();
			m_loadedRenderPass = Lamp::AssetManager::GetAsset<Lamp::RenderPass>(asset);
		});
}

void RenderPassEditorPanel::UpdateMainContent()
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open", "Ctrl + O"))
			{
				m_currentOverridePipeline = 0;
			}

			if (ImGui::MenuItem("New", "Ctrl + N"))
			{
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

void RenderPassEditorPanel::UpdateContent()
{
	UpdateEditor();

	if (m_assetBrowser->Begin())
	{
		m_assetBrowser->UpdateMainContent();
		m_assetBrowser->End();
	}
}

void RenderPassEditorPanel::UpdateEditor()
{
	ImGui::Begin("Editor##renderPass");
	if (m_loadedRenderPass)
	{
		UI::PushId();
		if (UI::BeginProperties())
		{
			UI::Property("Name", m_loadedRenderPass->name);

			// Override pipeline
			{
				std::vector<std::string> items;
				items.emplace_back("None");
				for (const auto& [name, pass] : Lamp::RenderPipelineRegistry::GetAllPipelines())
				{
					items.emplace_back(name);
				}

				if (UI::ComboProperty("Override Pipeline", m_currentOverridePipeline, items) && m_currentOverridePipeline != 0)
				{
					m_loadedRenderPass->overridePipeline = Lamp::RenderPipelineRegistry::Get(items[m_currentOverridePipeline])->GetGraphicsPipeline();
					m_loadedRenderPass->overridePipelineName = items[m_currentOverridePipeline];
				}

				// Draw type
				{
					const std::vector<const char*> items = { "Fullscreen Quad", "Opaque" };
					UI::ComboProperty("Draw Type", *(int32_t*)&m_loadedRenderPass->drawType, items);
				}

				UI::Property("Priority", m_loadedRenderPass->priority);
				UI::Property("Resizable", m_loadedRenderPass->resizeable);

				UI::EndProperties();
			}

			if (ImGui::CollapsingHeader("Framebuffer"))
			{
				Lamp::FramebufferSpecification& spec = const_cast<Lamp::FramebufferSpecification&>(m_loadedRenderPass->framebuffer->GetSpecification());

				UI::PushId();
				if (UI::BeginProperties())
				{
					UI::Property("Width", spec.width);
					UI::Property("Height", spec.height);

					UI::EndProperties();
				}
				UI::PopId();

				if (ImGui::CollapsingHeader("Attachments"))
				{
					uint32_t i = 0;
					for (auto& attachment : spec.attachments)
					{
						std::string attId = "Attachment##" + std::to_string(i);
						if (ImGui::CollapsingHeader(attId.c_str()))
						{
							UI::PushId();
							if (UI::BeginProperties())
							{
								// Format
								{
									const std::vector<const char*> items =
									{
										"R32F", "R32SI", "R32UI", "RGB", "RGBA", "RGBA16F", "RGBA32F", "RG16F", "RG32F", "SRGB", "BC1", "BC1SRGB",
										"BC2", "BC2SRGB", "BC3", "BC3SRGB", "BC4", "BC5", "BC7SRGB", "DEPTH32F", "DEPTH24STENCIL8"
									};

									UI::ComboProperty("Format", *(int32_t*)&attachment.format, items);
								}

								// Filter
								{
									const std::vector<const char*> items = { "None", "Linear", "Nearest" };
									UI::ComboProperty("Filter", *(int32_t*)&attachment.filterMode, items);
								}

								// Wrapping
								{
									const std::vector<const char*> items = { "None", "Clamp", "Repeat" };
									UI::ComboProperty("Wrapping", *(int32_t*)&attachment.wrapMode, items);
								}

								// Blending
								{
									const std::vector<const char*> items = { "None", "Min", "Max" };
									UI::ComboProperty("Blending", *(int32_t*)&attachment.blendMode, items);
								}

								// Clear mode
								{
									const std::vector<const char*> items = { "Clear", "Load", "DontCare" };
									UI::ComboProperty("Clear Mode", *(int32_t*)&attachment.clearMode, items);
								}

								UI::Property("Border Color", attachment.borderColor);
								UI::Property("Clear Color", attachment.clearColor);
								UI::Property("Copyable", attachment.copyable);
								UI::Property("Debug Name", attachment.debugName);

								UI::EndProperties();
							}
							UI::PopId();
						}
						i++;
					}
				}

			}
		}

		UI::PopId();
	}
	ImGui::End();
}

void RenderPassEditorPanel::Save()
{
	if (m_loadedRenderPass)
	{
		if (m_loadedRenderPass->path.empty())
		{
			SaveAs();
		}
		else
		{
			Lamp::AssetManager::Get().SaveAsset(m_loadedRenderPass);

			const std::string content = "Saved render pass \"" + m_loadedRenderPass->name + "\" to: " + FileSystem::GetPathRelativeToBaseFolder(m_loadedRenderPass->path).string();
			UI::Notify(NotificationType::Success, "Saved!", content);
		}
	}
}

void RenderPassEditorPanel::SaveAs()
{
	std::filesystem::path path = FileSystem::SaveFile("Render Pass (*.lprp)\0*.lprp\0");
	if (!path.empty())
	{
		if (path.extension().string() != ".lprp")
		{
			path += ".lprp";
		}

		Ref<Lamp::RenderPass> newPass = CreateRef<Lamp::RenderPass>(*m_loadedRenderPass);
		m_loadedRenderPass = newPass;

		m_loadedRenderPass->path = path;
		Lamp::AssetManager::Get().SaveAsset(m_loadedRenderPass);

		const std::string content = "Saved render pipeline \"" + m_loadedRenderPass->name + "\" to: " + FileSystem::GetPathRelativeToBaseFolder(m_loadedRenderPass->path).string();
		UI::Notify(NotificationType::Success, "Saved!", content);
	}
}
