#include "lppch.h"
#include "Image2D.h"

#include "Lamp/Log/Log.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"

#include "Lamp/Rendering/Renderer.h"
#include "Lamp/Rendering/Texture/SamplerLibrary.h"

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
				usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}
		}
		else if (m_specification.usage == ImageUsage::AttachmentStorage)
		{
			if (Utility::IsDepthFormat(m_specification.format))
			{
				usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			}
			else
			{
				usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
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
		imageInfo.flags = 0;

		if (m_specification.isCubeMap)
		{
			imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		}

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

				auto* mappedData = allocator.MapMemory<void>(stagingBufferAllocation);
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
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

		if (m_specification.isCubeMap)
		{
			imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		}
		else if (m_specification.layers > 1)
		{
			imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		}
		else if (m_specification.isCubeMap && m_specification.layers > 6)
		{
			imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
		}

		imageViewInfo.format = m_format;
		imageViewInfo.flags = 0;
		imageViewInfo.subresourceRange = {};
		imageViewInfo.subresourceRange.aspectMask = aspectMask;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.levelCount = m_specification.mips;
		imageViewInfo.subresourceRange.layerCount = m_specification.layers;
		imageViewInfo.image = m_image;

		LP_VK_CHECK(vkCreateImageView(device->GetHandle(), &imageViewInfo, nullptr, &m_imageViews[0]));

		m_sampler = SamplerLibrary::Get(m_specification.filter, m_specification.filter, m_specification.filter, m_specification.wrap, m_specification.compareOp, m_specification.anisoLevel);
	}

	void Image2D::Release()
	{
		if (!m_image && !m_bufferAllocation)
		{
			return;
		}

		Renderer::SubmitResourceFree([imageViews = m_imageViews, image = m_image, bufferAllocation = m_bufferAllocation]()
			{
				auto device = GraphicsContext::GetDevice();

				for (auto& imageView : imageViews)
				{
					vkDestroyImageView(device->GetHandle(), imageView.second, nullptr);
				}

				VulkanAllocator allocator{ "Image2D - Destroy" };
				allocator.DestroyImage(image, bufferAllocation);
			});

		m_imageViews.clear();
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

		VkImageSubresourceRange subresourceRange{};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.layerCount = m_specification.layers;
		subresourceRange.levelCount = m_specification.mips;

		Utility::TransitionImageLayout(commandBuffer, m_image, m_imageLayout, targetLayout, subresourceRange);
		m_imageLayout = targetLayout;
	}

	void Image2D::GenerateMips(bool readOnly, VkCommandBuffer commandBuffer)
	{
		auto device = GraphicsContext::GetDevice();
		VkCommandBuffer cmdBuffer = nullptr;

		if (!commandBuffer)
		{
			cmdBuffer = device->GetThreadSafeCommandBuffer(true);
		}
		else
		{
			cmdBuffer = commandBuffer;
		}

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = m_image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.oldLayout = m_imageLayout;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

		const uint32_t mipLevels = GetMipCount();
		m_specification.mips = mipLevels;

		for (uint32_t layer = 0; layer < m_specification.layers; layer++)
		{
			barrier.subresourceRange.baseArrayLayer = layer;
			vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}

		for (uint32_t i = 1; i < mipLevels; i++)
		{
			for (uint32_t layer = 0; layer < m_specification.layers; layer++)
			{
				VkImageBlit imageBlit{};
				imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.srcSubresource.layerCount = 1;
				imageBlit.srcSubresource.mipLevel = i - 1;
				imageBlit.srcSubresource.baseArrayLayer = layer;

				imageBlit.srcOffsets[0] = { 0, 0, 0 };
				imageBlit.srcOffsets[1] = { int32_t(m_specification.width >> (i - 1)), int32_t(m_specification.height >> (i - 1)), 1 };

				imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.dstSubresource.layerCount = 1;
				imageBlit.dstSubresource.mipLevel = i;
				imageBlit.dstSubresource.baseArrayLayer = layer;

				imageBlit.srcOffsets[0] = { 0, 0, 0 };
				imageBlit.srcOffsets[1] = { int32_t(m_specification.width >> i), int32_t(m_specification.height >> i), 1 };

				barrier.subresourceRange.baseMipLevel = i;
				barrier.subresourceRange.baseMipLevel = layer;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.layerCount = 1;

				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
				vkCmdBlitImage(cmdBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

				vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			}
		}

		VkImageSubresourceRange subresourceRange{};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.layerCount = m_specification.layers;
		subresourceRange.levelCount = m_specification.mips;

		const VkImageLayout targetLayout = readOnly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;

		Utility::TransitionImageLayout(cmdBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, targetLayout, subresourceRange);

		if (!commandBuffer)
		{
			device->FlushThreadSafeCommandBuffer(cmdBuffer);
		}

		m_hasGeneratedMips = true;
		m_imageLayout = targetLayout;
	}

	VkImageView Image2D::CreateMipView(uint32_t mip)
	{
		if (m_imageViews.find(mip) != m_imageViews.end())
		{
			return m_imageViews.at(mip);
		}

		VkImageViewCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;

		if (m_specification.isCubeMap)
		{
			info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		}
		else if (m_specification.layers > 1)
		{
			info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		}
		else if (m_specification.isCubeMap && m_specification.layers > 6)
		{
			info.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
		}

		info.format = m_format;
		info.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.baseMipLevel = mip;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = m_specification.layers;
		info.subresourceRange.levelCount = 1;
		info.image = m_image;

		auto device = GraphicsContext::GetDevice();
		LP_VK_CHECK(vkCreateImageView(device->GetHandle(), &info, nullptr, &m_imageViews[mip]));

		return m_imageViews.at(mip);
	}

	const uint32_t Image2D::GetMipCount() const
	{
		return Utility::CalculateMipCount(m_specification.width, m_specification.height);
	}

	Ref<Image2D> Image2D::Create(const ImageSpecification& specification, const void* data)
	{
		return CreateRef<Image2D>(specification, data);
	}
}