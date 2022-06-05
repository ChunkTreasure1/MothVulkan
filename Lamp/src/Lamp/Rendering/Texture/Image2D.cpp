#include "lppch.h"
#include "Image2D.h"

#include "Lamp/Log/Log.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"

#include "Lamp/Utility/ImageUtility.h"

namespace Lamp
{
	Image2D::Image2D(const ImageSpecification& specification, const void* data)
		: m_specification(specification)
	{
		Invalidate(data);
	}

	Image2D::~Image2D()
	{
		Release();
	}

	void Image2D::Invalidate(const void* data)
	{
		Release();

		VulkanAllocator allocator{ "Image2D - Create" };
		auto device = GraphicsContext::GetDevice();

		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
		if (m_specification.copyable)
		{
			usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}

		if (m_specification.usage == ImageUsage::Attachment)
		{
			if (Utility::IsDepthFormat(m_specification.format))
			{
				usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			}
			else
			{
				usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			}
		}
		else if (m_specification.usage == ImageUsage::Texture)
		{
			usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}
		else if (m_specification.usage == ImageUsage::Storage)
		{
			usage |= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		VkImageAspectFlags aspectMask = Utility::IsDepthFormat(m_specification.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		if (m_specification.format == ImageFormat::DEPTH24STENCIL8)
		{
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		m_format = Utility::LampToVulkanFormat(m_specification.format);

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.usage = usage;
		imageInfo.extent.width = m_specification.width;
		imageInfo.extent.height = m_specification.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = m_specification.mips;
		imageInfo.arrayLayers = m_specification.layers;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.format = m_format;

		m_bufferAllocation = allocator.AllocateImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY, m_image);

		if (data)
		{
			auto device = GraphicsContext::GetDevice();

			VkBuffer stagingBuffer;
			VmaAllocation stagingBufferAllocation;

			VkDeviceSize bufferSize = m_specification.width * m_specification.height * Utility::PerPixelSizeFromFormat(m_specification.format);

			// Staging buffer
			{
				VkBufferCreateInfo bufferInfo{};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = bufferSize;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				stagingBufferAllocation = allocator.AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer);

				void* mappedData = allocator.MapMemory<void>(stagingBufferAllocation);
				memcpy_s(mappedData, bufferSize, data, bufferSize);
				allocator.UnmapMemory(stagingBufferAllocation);

				Utility::TransitionImageLayout(m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
				Utility::CopyBufferToImage(stagingBuffer, m_image, m_specification.width, m_specification.height);
				Utility::TransitionImageLayout(m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

				allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
			}
		}

		VkImageViewCreateInfo imageViewInfo{};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.viewType = m_specification.layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = m_format;
		imageViewInfo.flags = 0;
		imageViewInfo.subresourceRange = {};
		imageViewInfo.subresourceRange.aspectMask = aspectMask;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = m_specification.mips;
		imageViewInfo.subresourceRange.layerCount = m_specification.layers;
		imageViewInfo.image = m_image;

		LP_VK_CHECK(vkCreateImageView(device->GetHandle(), &imageViewInfo, nullptr, &m_imageViews[0]));

		///////////////////////MOVE TO ANOTHER PLACE/////////////////////////////
		VkSamplerCreateInfo samplerCreateInfo{};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.anisotropyEnable = m_specification.anisoLevel != AniostopyLevel::None ? VK_TRUE : VK_FALSE;
		samplerCreateInfo.maxAnisotropy = (float)m_specification.anisoLevel; // TODO: Add capabilities checking!
		samplerCreateInfo.magFilter = Utility::LampToVulkanFilter(m_specification.filter);
		samplerCreateInfo.minFilter = Utility::LampToVulkanFilter(m_specification.filter);
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = Utility::LampToVulkanWrap(m_specification.wrap);
		samplerCreateInfo.addressModeV = Utility::LampToVulkanWrap(m_specification.wrap);
		samplerCreateInfo.addressModeW = Utility::LampToVulkanWrap(m_specification.wrap);
		samplerCreateInfo.mipLodBias = 0.f;
		samplerCreateInfo.minLod = 0.f;
		samplerCreateInfo.maxLod = 3.f;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		samplerCreateInfo.compareEnable = m_specification.comparable ? VK_TRUE : VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_LESS;

		LP_VK_CHECK(vkCreateSampler(device->GetHandle(), &samplerCreateInfo, nullptr, &m_sampler));
	}

	void Image2D::Release()
	{
		if (!m_image && !m_bufferAllocation)
		{
			return;
		}

		auto device = GraphicsContext::GetDevice();

		vkDestroySampler(device->GetHandle(), m_sampler, nullptr);

		for (auto& imageView : m_imageViews)
		{
			vkDestroyImageView(device->GetHandle(), imageView.second, nullptr);
		}
		m_imageViews.clear();

		VulkanAllocator allocator{ "Image2D - Destroy" };
		allocator.DestroyImage(m_image, m_bufferAllocation);

		m_image = nullptr;
		m_bufferAllocation = nullptr;
	}

	void Image2D::TransitionToLayout(VkCommandBuffer commandBuffer, VkImageLayout targetLayout)
	{
		if (m_imageLayout == targetLayout)
		{
			return;
		}

		const VkImageAspectFlags flag = Utility::IsDepthFormat(m_specification.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		const VkImageSubresourceRange range = { flag, 0, 1, 0, 1 };

		Utility::TransitionImageLayout(commandBuffer, m_image, m_imageLayout, targetLayout, range);
		m_imageLayout = targetLayout;
	}

	Ref<Image2D> Image2D::Create(const ImageSpecification& specification, const void* data)
	{
		return CreateRef<Image2D>(specification, data);
	}
}