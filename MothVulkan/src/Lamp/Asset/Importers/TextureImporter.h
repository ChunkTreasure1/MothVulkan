#pragma once

#include "Lamp/Core/Base.h"

#include <filesystem>

namespace Lamp
{
	class Texture2D;
	class TextureImporter
	{
	public:
		virtual ~TextureImporter() = default;

		static void Initialize();
		static void Shutdown();

		static Ref<Texture2D> ImportTexture(const std::filesystem::path& path);

	protected:
		virtual Ref<Texture2D> ImportTextureImpl(const std::filesystem::path& path) = 0;

	private:
		enum class TextureFormat
		{
			KTX,
			DDS,
			Other
		};

		static TextureFormat FormatFromExtension(const std::filesystem::path& path);
		inline static std::unordered_map<TextureFormat, Scope<TextureImporter>> s_importers;
	};
}