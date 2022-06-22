#pragma once

#include "Lamp/Core/Graphics/VulkanAllocator.h"
#include "Lamp/Rendering/Texture/ImageCommon.h"

#include <map>

namespace Lamp
{
	class Image2D
	{
	public:
		Image2D(const ImageSpecification& specification, const void* data = nullptr);
		~Image2D();

		void Invalidate(const void* data = nullptr);
		void Release();

		void TransitionToLayout(VkCommandBuffer commandBuffer, VkImageLayout targetLayout);
		void GenerateMips(bool readOnly);

		inline const VkImage GetHandle() const { return m_image; };
		inline const ImageFormat GetFormat() const { return m_specification.format; }
		inline const ImageSpecification& GetSpecification() const { return m_specification; }

		inline const uint32_t GetWidth() const { return m_specification.width; }
		inline const uint32_t GetHeight() const { return m_specification.height; }
		const uint32_t GetMipCount() const;

		inline const VkImageView GetView(uint32_t index = 0) const { return m_imageViews.at(index); }
		inline const VkSampler GetSampler() const { return m_sampler; }
		inline const VkImageLayout GetLayout() const { return m_imageLayout; }

		static Ref<Image2D> Create(const ImageSpecification& specification, const void* data = nullptr);

	private:
		ImageSpecification m_specification;

		VmaAllocation m_bufferAllocation = nullptr;
		VkImage m_image = nullptr;

		VkFormat m_format = VK_FORMAT_R8G8B8A8_UNORM;
		VkImageLayout m_imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkSampler m_sampler;

		std::map<uint32_t, VkImageView> m_imageViews;
		bool m_hasGeneratedMips = false;
	};
}