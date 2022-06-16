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

	class TextureSourceImporter : public AssetImporter
	{
	public:
		bool Load(const std::filesystem::path& path, Ref<Asset>& asset) const override;
		void Save(const Ref<Asset>& asset) const override;		
	};

	class ShaderImporter : public AssetImporter
	{
	public:
		bool Load(const std::filesystem::path& path, Ref<Asset>& asset) const override;
		void Save(const Ref<Asset>& asset) const override;
	};

	class MultiMaterialImporter : public AssetImporter
	{
	public:
		bool Load(const std::filesystem::path& path, Ref<Asset>& asset) const override;
		void Save(const Ref<Asset>& asset) const override;
	};
}