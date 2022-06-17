#pragma once

#include "Lamp/Asset/Importers/TextureImporter.h"

namespace Lamp
{
	class DDSTextureImporter : public TextureImporter
	{
	public:
		~DDSTextureImporter() override = default;

	protected:
		Ref<Texture2D> ImportTextureImpl(const std::filesystem::path& path) override;
	};
}