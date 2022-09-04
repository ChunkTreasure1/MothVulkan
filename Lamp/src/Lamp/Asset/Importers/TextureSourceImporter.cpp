#include "lppch.h"
#include "TextureSourceImporter.h"

#include "Lamp/Asset/Importers/TextureImporter.h"
#include "Lamp/Rendering/Texture/Image2D.h"
#include "Lamp/Rendering/Texture/Texture2D.h"
#include "Lamp/Log/Log.h"

#include <savedds.h>

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
		const Ref<Texture2D> texture = std::reinterpret_pointer_cast<Texture2D>(asset);
		const Ref<Image2D> image = texture->GetImage();
		const Buffer& buffer = image->GetData();

		if (!buffer.IsValid())
		{
			return;
		}

		savedds(asset->path.string().c_str(), buffer.As<uint8_t>(), (int32_t)image->GetWidth(), (int32_t)image->GetHeight(), 32);
	}
}