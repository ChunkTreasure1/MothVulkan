#include "lppch.h"
#include "AssetImporter.h"

#include "Lamp/Log/Log.h"
#include "Lamp/Asset/Importers/TextureImporter.h"

#include "Lamp/Rendering/Texture/Texture2D.h"

namespace Lamp
{
	bool TextureSourceImporter::Load(const std::filesystem::path& path, Ref<Asset>& asset) const
	{
		asset = CreateRef<Texture2D>();

		if (!std::filesystem::exists(path)) [[unlikely]]
		{
			LP_CORE_ERROR("File {0} not found!", path.string().c_str());
			asset->SetFlag(AssetFlag::Missing, true);
			return false;
		}
		auto mesh = TextureImporter::ImportTexture(path);

		if (!mesh) [[unlikely]]
		{
			asset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		asset = mesh;
		asset->path = path;
		return true;
	}

	void TextureSourceImporter::Save(const Ref<Asset>& asset) const
	{
	}

	bool ShaderImporter::Load(const std::filesystem::path& path, Ref<Asset>& asset) const
	{
		return false;
	}

	void ShaderImporter::Save(const Ref<Asset>& asset) const
	{
		
	}
}