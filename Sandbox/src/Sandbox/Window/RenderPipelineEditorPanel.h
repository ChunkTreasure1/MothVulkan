#pragma once

#include "Sandbox/Window/EditorWindow.h"

namespace Lamp
{
	class RenderPipeline;
}

class RenderPipelineEditorPanel : public EditorWindow
{
public:
	RenderPipelineEditorPanel();

	void UpdateMainContent() override;

private:
	Ref<Lamp::RenderPipeline> m_loadedRenderPipeline;
};