#pragma once

#include "Lamp/Asset/Importers/TextureImporter.h"

namespace Lamp
{
	class DefaultTextureImporter : public TextureImporter
	{
	public:
		~DefaultTextureImporter() override = default;

	protected:
		Ref<Texture2D> ImportTextureImpl(const std::filesystem::path& path) override;
	};
}