#include "lppch.h"
#include "TextureImporter.h"

#include "DefaultTextureImporter.h"

namespace Lamp
{
	void TextureImporter::Initialize()
	{
		s_importers[TextureFormat::Other] = CreateScope<DefaultTextureImporter>();
	}

	void TextureImporter::Shutdown()
	{
		s_importers.clear();
	}

	Ref<Texture2D> TextureImporter::ImportTexture(const std::filesystem::path& path)
	{
		return s_importers[FormatFromExtension(path)]->ImportTextureImpl(path);
	}

	Ref<Texture2D> TextureImporter::ImportTextureImpl(const std::filesystem::path& path)
	{
		return Ref<Texture2D>();
	}

	TextureImporter::TextureFormat TextureImporter::FormatFromExtension(const std::filesystem::path& path)
	{
		auto ext = path.extension().string();
		
		if (ext == ".dds" || ext == ".DDS")
		{
			return TextureFormat::DDS;
		}
		else if (ext == ".ktx" || ext == ".ktx2")
		{
			return TextureFormat::KTX;
		}
		
		return TextureFormat::Other;
	}

}