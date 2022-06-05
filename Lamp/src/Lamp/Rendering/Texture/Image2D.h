#pragma once

#include "Lamp/Core/Graphics/VulkanAllocator.h"

#include <map>
#include <cstdint>

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
		Storage
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
	};

	class Image2D
	{
	public:
		Image2D(const ImageSpecification& specification, const void* data = nullptr);
		~Image2D();

		void Invalidate(const void* data = nullptr);
		void Release();

		void TransitionToLayout(VkCommandBuffer commandBuffer, VkImageLayout targetLayout);

		inline const VkImage GetHandle() const { return m_image; };
		inline const uint32_t GetWidth() const { return m_specification.width; }
		inline const uint32_t GetHeight() const { return m_specification.height; }
		inline const ImageFormat GetFormat() const { return m_specification.format; }
		inline const ImageSpecification& GetSpecification() const { return m_specification; }

		inline const VkImageView GetView(uint32_t index = 0) const { return m_imageViews.at(index); }
		inline const VkSampler GetSampler() const { return m_sampler; }

		static Ref<Image2D> Create(const ImageSpecification& specification, const void* data = nullptr);

	private:
		ImageSpecification m_specification;

		VmaAllocation m_bufferAllocation = nullptr;
		VkImage m_image = nullptr;

		VkFormat m_format = VK_FORMAT_R8G8B8A8_UNORM;
		VkImageLayout m_imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;;
		VkSampler m_sampler;

		std::map<uint32_t, VkImageView> m_imageViews;
	};
}