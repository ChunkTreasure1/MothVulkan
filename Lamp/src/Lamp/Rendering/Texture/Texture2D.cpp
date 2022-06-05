#include "lppch.h"
#include "Texture2D.h"

namespace Lamp
{
	Texture2D::Texture2D(ImageFormat format, uint32_t width, uint32_t height, void* data)
	{
		ImageSpecification imageSpec{};
		imageSpec.format = format;
		imageSpec.usage = ImageUsage::Texture;
		imageSpec.width = (uint32_t)width;
		imageSpec.height = (uint32_t)height;

		m_image = Image2D::Create(imageSpec, data);
	}

	Texture2D::~Texture2D()
	{
		m_image = nullptr;
	}

	void Texture2D::SetData(const void* data, uint32_t size)
	{
		
	}

	const uint32_t Texture2D::GetWidth() const
	{
		return m_image->GetWidth();
	}

	const uint32_t Texture2D::GetHeight() const
	{
		return m_image->GetHeight();
	}

	Ref<Texture2D> Texture2D::Create(ImageFormat format, uint32_t width, uint32_t height, void* data)
	{
		return CreateRef<Texture2D>(format, width, height, data);
	}
}