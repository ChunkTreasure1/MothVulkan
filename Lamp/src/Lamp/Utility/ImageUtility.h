#pragma once

#include "Lamp/Core/Graphics/GraphicsDevice.h"
#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Log/Log.h"

#include "Lamp/Rendering/Texture/Image2D.h"

namespace Lamp
{
	namespace Utility
	{
		inline void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout currentLayout, VkImageLayout targetLayout, VkImageSubresourceRange subresource)
		{
			// Create an image barrier object
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.oldLayout = currentLayout;
			imageMemoryBarrier.newLayout = targetLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresource;

			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
			switch (currentLayout)
			{
				case VK_IMAGE_LAYOUT_UNDEFINED:
					// Image layout is undefined (or does not matter)
					// Only valid as initial layout
					// No flags required, listed only for completeness
					imageMemoryBarrier.srcAccessMask = 0;
					break;

				case VK_IMAGE_LAYOUT_PREINITIALIZED:
					// Image is preinitialized
					// Only valid as initial layout for linear images, preserves memory contents
					// Make sure host writes have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
					// Image is a color attachment
					// Make sure any writes to the color buffer have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
					// Image is a depth/stencil attachment
					// Make sure any writes to the depth/stencil buffer have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
					// Image is a transfer source
					// Make sure any reads from the image have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					break;

				case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
					// Image is a transfer destination
					// Make sure any writes to the image have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
					// Image is read by a shader
					// Make sure any shader reads from the image have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
					break;
				default:
					// Other source layouts aren't handled (yet)
					break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (targetLayout)
			{
				case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
					// Image will be used as a transfer destination
					// Make sure any writes to the image have been finished
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
					// Image will be used as a transfer source
					// Make sure any reads from the image have been finished
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					break;

				case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
					// Image will be used as a color attachment
					// Make sure any writes to the color buffer have been finished
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
					// Image layout will be used as a depth/stencil attachment
					// Make sure any writes to depth/stencil buffer have been finished
					imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
					// Image will be read in a shader (sampler, input attachment)
					// Make sure any writes to the image have been finished
					if (imageMemoryBarrier.srcAccessMask == 0)
					{
						imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
					}
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					break;
				default:
					// Other source layouts aren't handled (yet)
					break;
			}

			VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			VkPipelineStageFlags destStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

			if (currentLayout == VK_IMAGE_LAYOUT_UNDEFINED && targetLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (currentLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && targetLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else if (currentLayout == VK_IMAGE_LAYOUT_UNDEFINED && targetLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			{
				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			}
			else if (currentLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && targetLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else if (currentLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && targetLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			{
				sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				destStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (currentLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && targetLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			{
				sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				destStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (currentLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && targetLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			{
				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			}
			else if (currentLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && targetLayout == VK_IMAGE_LAYOUT_GENERAL)
			{
				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}

			// Put barrier inside setup command buffer
			vkCmdPipelineBarrier(commandBuffer, sourceStage, destStage, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		}

		inline void TransitionImageLayout(VkImage image, VkImageLayout currentLayout, VkImageLayout targetLayout)
		{
			auto device = GraphicsContext::GetDevice();
			
			VkImageSubresourceRange subresource{};
			subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresource.baseMipLevel = 0;
			subresource.levelCount = VK_REMAINING_MIP_LEVELS;
			subresource.baseArrayLayer = 0;
			subresource.layerCount = VK_REMAINING_ARRAY_LAYERS;

			auto commandBuffer = device->GetThreadSafeCommandBuffer(true);
			
			TransitionImageLayout(commandBuffer, image, currentLayout, targetLayout, subresource);
			
			device->FlushThreadSafeCommandBuffer(commandBuffer);
		}

		inline void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t mipLevel = 0)
		{
			auto device = GraphicsContext::GetDevice();
			VkCommandBuffer cmdBuffer = device->GetThreadSafeCommandBuffer(true);
			
			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = mipLevel;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;

			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = { width, height, 1 };

			vkCmdCopyBufferToImage(cmdBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			device->FlushThreadSafeCommandBuffer(cmdBuffer);
		}

		inline void CopyImageToBuffer(VkBuffer dstBuffer, VkImage srcImage, VkImageLayout imageLayout, uint32_t width, uint32_t height, uint32_t mipLevel = 0)
		{
			auto device = GraphicsContext::GetDevice();
			VkCommandBuffer cmdBuffer = device->GetThreadSafeCommandBuffer(true);

			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = mipLevel;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;

			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = { width, height, 1 };

			vkCmdCopyImageToBuffer(cmdBuffer, srcImage, imageLayout, dstBuffer, 1, &region);
			device->FlushThreadSafeCommandBuffer(cmdBuffer);
		}

		inline void GenerateMipMaps(VkImage image, uint32_t width, uint32_t height, uint32_t mipLevels)
		{
			auto device = GraphicsContext::GetDevice();
			VkCommandBuffer cmdBuffer = device->GetThreadSafeCommandBuffer(true);

			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.subresourceRange.levelCount = 1;

			int32_t mipWidth = (int32_t)width;
			int32_t mipHeight = (int32_t)height;

			for (uint32_t i = 1; i < mipLevels; i++)
			{
				barrier.subresourceRange.baseMipLevel = i - 1;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

				VkImageBlit blit{};
				blit.srcOffsets[0] = { 0, 0, 0 };
				blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
				blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcSubresource.baseArrayLayer = 0;
				blit.srcSubresource.layerCount = 1;
				blit.dstOffsets[0] = { 0, 0, 0 };
				blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
				blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.dstSubresource.mipLevel = i;
				blit.dstSubresource.baseArrayLayer = 0;
				blit.dstSubresource.layerCount = 1;

				vkCmdBlitImage(cmdBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

				if (mipWidth > 1)
				{
					mipWidth /= 2;
				}

				if (mipHeight > 1)
				{
					mipHeight /= 2;
				}
			}

			barrier.subresourceRange.baseMipLevel = mipLevels - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(cmdBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			device->FlushThreadSafeCommandBuffer(cmdBuffer);
		}

		inline void InsertImageMemoryBarrier(VkCommandBuffer commandBuffer, VkImage image, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageSubresourceRange subResourceRange)
		{
			VkImageMemoryBarrier imageMemBarrier{};
			imageMemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemBarrier.srcAccessMask = srcAccess;
			imageMemBarrier.dstAccessMask = dstAccess;
			imageMemBarrier.oldLayout = oldLayout;
			imageMemBarrier.newLayout = newLayout;
			imageMemBarrier.image = image;
			imageMemBarrier.subresourceRange = subResourceRange;

			vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemBarrier);
		}

		///////////////////////////Conversions////////////////////////////
		inline bool IsDepthFormat(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::DEPTH32F: return true;
				case ImageFormat::DEPTH24STENCIL8: return true;
			}

			return false;
		}

		inline VkFormat LampToVulkanFormat(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::R32F: return VK_FORMAT_R32_SFLOAT;
				case ImageFormat::R32SI: return VK_FORMAT_R32_SINT;
				case ImageFormat::R32UI: return VK_FORMAT_R32_UINT;

				case ImageFormat::RGB: return VK_FORMAT_R8G8B8_UNORM;
				case ImageFormat::RGBA: return VK_FORMAT_R8G8B8A8_UNORM;
				
				case ImageFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
				case ImageFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
				
				case ImageFormat::RG16F: return VK_FORMAT_R16G16_SFLOAT;
				case ImageFormat::RG32F: return VK_FORMAT_R32G32B32_SFLOAT;
				
				case ImageFormat::SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
				
				case ImageFormat::BC1: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
				case ImageFormat::BC1SRGB: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;

				case ImageFormat::BC2: return VK_FORMAT_BC2_UNORM_BLOCK;
				case ImageFormat::BC2SRGB: return VK_FORMAT_BC2_SRGB_BLOCK;

				case ImageFormat::BC3: return VK_FORMAT_BC3_UNORM_BLOCK;
				case ImageFormat::BC3SRGB: return VK_FORMAT_BC3_SRGB_BLOCK;

				case ImageFormat::BC4: return VK_FORMAT_BC4_UNORM_BLOCK;
				case ImageFormat::BC5: return VK_FORMAT_BC5_UNORM_BLOCK;

				case ImageFormat::BC7: return VK_FORMAT_BC7_UNORM_BLOCK;
				case ImageFormat::BC7SRGB: return VK_FORMAT_BC7_SRGB_BLOCK;					

				case ImageFormat::DEPTH32F: return VK_FORMAT_D32_SFLOAT;
				case ImageFormat::DEPTH24STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;

				default:
					break;
			}

			return (VkFormat)0;
		}

		inline VkFilter LampToVulkanFilter(TextureFilter filter)
		{
			switch (filter)
			{
				case TextureFilter::None: LP_CORE_ASSERT(false, "Filter must be chosen!") return VK_FILTER_LINEAR;
				case TextureFilter::Linear: return VK_FILTER_LINEAR;
				case TextureFilter::Nearest: return VK_FILTER_NEAREST;
			}

			return VK_FILTER_LINEAR;
		}
		
		inline VkSamplerAddressMode LampToVulkanWrap(TextureWrap wrap)
		{
			switch (wrap)
			{
				case TextureWrap::None: LP_CORE_ASSERT(false, "Wrap mode must be set!"); break;
				case TextureWrap::Clamp: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				case TextureWrap::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			}
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;			
		}

		inline uint32_t PerPixelSizeFromFormat(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::None: return 0;
				case ImageFormat::R32F: return 1 * 4;
				case ImageFormat::R32SI: return 1 * 4;
				case ImageFormat::R32UI: return 1 * 4;
				case ImageFormat::RGB: return 3 * 1;
				case ImageFormat::RGBA: return 4 * 1;
				case ImageFormat::RGBA16F: return 4 * 2;
				case ImageFormat::RGBA32F: return 4 * 4;
				case ImageFormat::RG16F: return 2 * 2;
				case ImageFormat::RG32F: return 2 * 4;
				case ImageFormat::SRGB: return 3 * 1;
				case ImageFormat::DEPTH32F: return 1 * 4;
				case ImageFormat::DEPTH24STENCIL8: return 4;
			}

			return 0;
		}
	
		static uint32_t CalculateMipCount(uint32_t width, uint32_t height)
		{
			return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
		}
	}
}