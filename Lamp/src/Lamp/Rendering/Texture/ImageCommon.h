#pragma once

#include <stdint.h>

namespace Lamp
{
	enum class ImageFormat : uint32_t
	{
		None = 0,
		R32F,
		R32SI,
		R32UI,
		RGB,
		RGBA,
		RGBA16F,
		RGBA32F,
		RG16F,
		RG32F,

		SRGB,

		BC1,
		BC1SRGB,

		BC2,
		BC2SRGB,

		BC3,
		BC3SRGB,

		BC4,

		BC5,

		BC7,
		BC7SRGB,


		DEPTH32F,
		DEPTH24STENCIL8
	};

	enum class AniostopyLevel : uint32_t
	{
		None = 0,
		X2 = 2,
		X4 = 4,
		X8 = 8,
		X16 = 16
	};

	enum class ImageUsage : uint32_t
	{
		None = 0,
		Texture,
		Attachment,
		Storage,
		AttachmentStorage
	};

	enum class TextureWrap : uint32_t
	{
		None = 0,
		Clamp,
		Repeat
	};

	enum class TextureFilter : uint32_t
	{
		None = 0,
		Linear,
		Nearest
	};

	enum class TextureBlend : uint32_t
	{
		None,
		Min,
		Max
	};

	enum class ImageDimension : uint32_t
	{
		Dim1D,
		Dim2D,
		Dim3D,
		DimCube
	};

	struct ImageSpecification
	{
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t mips = 1;
		uint32_t layers = 1;

		ImageFormat format = ImageFormat::RGBA;
		ImageUsage usage = ImageUsage::Texture;
		TextureWrap wrap = TextureWrap::Repeat;
		TextureFilter filter = TextureFilter::Linear;

		AniostopyLevel anisoLevel = AniostopyLevel::None;

		bool copyable = false;
		bool comparable = false;
		bool isCubeMap = false;
	};
}