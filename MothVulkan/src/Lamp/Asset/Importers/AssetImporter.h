#pragma once

#include "Lamp/Asset/Asset.h"

namespace Lamp
{
	class AssetImporter
	{
	public:
		virtual bool Load(const std::filesystem::path& path, Ref<Asset>& asset) const = 0;
		virtual void Save(const Ref<Asset>& asset) const = 0;
	};


}