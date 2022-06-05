#pragma once

#include "Lamp/Asset/Importers/AssetImporter.h"

namespace Lamp
{
	class RenderPassImporter : public AssetImporter
	{
	public:
		bool Load(const std::filesystem::path& path, Ref<Asset>& asset) const override;
		void Save(const Ref<Asset>& asset) const override;
	};
}