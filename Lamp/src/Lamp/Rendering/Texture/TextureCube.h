#pragma once

#include "Lamp/Asset/Asset.h"

#include "Lamp/Rendering/Texture/Image2D.h"

namespace Lamp
{
	class TextureCube : public Asset
	{
	public:
		TextureCube(ImageFormat format, uint32_t width, uint32_t height, const void* data);
		TextureCube() = default;
		~TextureCube() override;

		void SetData(const void* data, uint32_t size);

		const uint32_t GetWidth() const;
		const uint32_t GetHeight() const;

		inline const Ref<Image2D> GetImage() const { return m_image; }

		static AssetType GetStaticType() { return AssetType::Texture; }
		AssetType GetType() { return GetStaticType(); }

		static Ref<TextureCube> Create(ImageFormat format, uint32_t width, uint32_t height, const void* data = nullptr);

	private:
		friend class DefaultTextureImporter;
		friend class DDSTextureImporter;

		Ref<Image2D> m_image;
	};
}