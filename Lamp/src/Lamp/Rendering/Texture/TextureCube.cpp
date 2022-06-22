#include "lppch.h"
#include "TextureCube.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Utility/ImageUtility.h"

namespace Lamp
{
	TextureCube::TextureCube(ImageFormat format, uint32_t width, uint32_t height, const void* data)
	{
		ImageSpecification imageSpec{};
		imageSpec.format = format;
		imageSpec.usage = ImageUsage::Texture;
		imageSpec.width = (uint32_t)width;
		imageSpec.height = (uint32_t)height;
		imageSpec.layers = 6;

		m_image = Image2D::Create(imageSpec, data);
	}

	TextureCube::~TextureCube()
	{
		m_image = nullptr;
	}

	void TextureCube::SetData(const void* data, uint32_t size)
	{
	}

	const uint32_t TextureCube::GetWidth() const
	{
		return m_image->GetWidth();
	}

	const uint32_t TextureCube::GetHeight() const
	{
		return m_image->GetHeight();
	}

	Ref<TextureCube> TextureCube::Create(ImageFormat format, uint32_t width, uint32_t height, const void* data)
	{
		return CreateRef<TextureCube>(format, width, height, data);
	}
}