#include "sbpch.h"
#include "ViewportPanel.h"

#include <Lamp/Rendering/Framebuffer.h>
#include <Lamp/Utility/UIUtility.h>

ViewportPanel::ViewportPanel(Ref<Lamp::Framebuffer> outputBuffer)
	: EditorWindow("Viewport"), m_outputFramebuffer(outputBuffer)
{
	m_isOpen = true;
}

void ViewportPanel::UpdateContent()
{
	auto size = ImGui::GetContentRegionAvail();
	ImGui::Image(UI::GetTextureID(m_outputFramebuffer->GetColorAttachment(0)), size);
}