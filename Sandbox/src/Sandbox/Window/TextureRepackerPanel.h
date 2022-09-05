#pragma once

#include "Sandbox/Window/EditorWindow.h"

class TextureRepackerPanel : public EditorWindow
{
public:
	TextureRepackerPanel();

	void UpdateMainContent() override;

private:
	void RepackTexture(const std::filesystem::path& path);

	std::filesystem::path m_normalPaths;
	std::filesystem::path m_metallicPaths;
	std::filesystem::path m_rougnessPaths;
};