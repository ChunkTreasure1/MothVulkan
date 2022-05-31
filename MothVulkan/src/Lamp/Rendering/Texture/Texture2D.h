#pragma once

#include "Lamp/Asset/Asset.h"

#include "Lamp/Rendering/Texture/Image2D.h"

namespace Lamp
{
	class Texture2D : public Asset
	{
	public:
		Texture2D(ImageFormat format, uint32_t width, uint32_t height);
		Texture2D() = default;

		~Texture2D() override;

		const uint32_t GetWidth() const;
		const uint32_t GetHeight() const;

		inline const Ref<Image2D> GetImage() const { return m_image; }

		static AssetType GetStaticType() { return AssetType::Texture; }
		AssetType GetType() { return GetStaticType(); }		
		
		static Ref<Texture2D> Create(ImageFormat format, uint32_t width, uint32_t height);

	private:
		friend class DefaultTextureImporter;

		Ref<Image2D> m_image;
	};
}