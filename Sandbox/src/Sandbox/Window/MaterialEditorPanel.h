#pragma once

#include "Sandbox/Window/EditorWindow.h"

namespace Lamp
{
	class MultiMaterial;
	class Material;
}

class MaterialEditorPanel : public EditorWindow
{ 
public:
	MaterialEditorPanel();

	void UpdateMainContent() override;
	void UpdateContent() override;

private:	
	Ref<Lamp::MultiMaterial> m_selectedMaterial;
	Ref<Lamp::Material> m_selectedSubMaterial;
};