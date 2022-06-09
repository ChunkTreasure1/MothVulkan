#pragma once

#include "Sandbox/Window/EditorWindow.h"

namespace Lamp
{
	class Framebuffer;
}

class ViewportPanel : public EditorWindow
{
public:
	ViewportPanel(Ref<Lamp::Framebuffer> outputBuffer);

	void UpdateContent() override;

private:
	Ref<Lamp::Framebuffer> m_outputFramebuffer;
};