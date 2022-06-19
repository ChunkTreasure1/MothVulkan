 #include "lppch.h"
#include "DDSTextureImporter.h"

#include "Lamp/Log/Log.h"
#include "Lamp/Core/Graphics/VulkanAllocator.h"

#include "Lamp/Rendering/Texture/Image2D.h"
#include "Lamp/Rendering/Texture/Texture2D.h"

#include "Lamp/Utility/ImageUtility.h"

#include <vulkan/vulkan.h>

#define TINYDDSLOADER_IMPLEMENTATION
#include <tinyddsloader.h>

namespace tdl = tinyddsloader;

namespace Lamp
{
	namespace Utility
	{
		static void PrintDDSError(tdl::Result code, const std::filesystem::path& path)
		{
			switch (code)
			{
				case tdl::ErrorFileOpen: LP_CORE_ERROR("Unable to open texture {0}!", path.string().c_str()); break;
				case tdl::ErrorRead: LP_CORE_ERROR("Unable to read texture {0}!", path.string().c_str()); break;
				case tdl::ErrorMagicWord: LP_CORE_ERROR("Unable to read magic word in texture {0}!", path.string().c_str()); break;
				case tdl::ErrorSize: LP_CORE_ERROR("Size is wrong in texture {0}!", path.string().c_str()); break;
				case tdl::ErrorVerify: LP_CORE_ERROR("Unable to verify texture {0}!", path.string().c_str()); break;
				case tdl::ErrorNotSupported: LP_CORE_ERROR("Texture type of texture {0} not supported!", path.string().c_str()); break;
				case tdl::ErrorInvalidData: LP_CORE_ERROR("Invalid data in texture {0}!", path.string().c_str()); break;
			}
		}

		static ImageFormat DDSToLampImageFormat(tdl::DDSFile::DXGIFormat format)
		{
			switch (format)
			{
				case tdl::DDSFile::DXGIFormat::R32G32B32A32_Float: return ImageFormat::RGBA32F;
				case tdl::DDSFile::DXGIFormat::R16G16B16A16_Float: return ImageFormat::RGBA16F;
				
				case tdl::DDSFile::DXGIFormat::R32G32_Float: return ImageFormat::RG32F;
				case tdl::DDSFile::DXGIFormat::R16G16_Float: return ImageFormat::RG16F;
				
				case tdl::DDSFile::DXGIFormat::R8G8B8A8_UNorm: return ImageFormat::RGBA;
				case tdl::DDSFile::DXGIFormat::R8G8B8A8_UNorm_SRGB: return ImageFormat::SRGB;
					
				case tdl::DDSFile::DXGIFormat::R32_Float: return ImageFormat::R32F;
				case tdl::DDSFile::DXGIFormat::R32_UInt: return ImageFormat::R32UI;
				case tdl::DDSFile::DXGIFormat::R32_SInt: return ImageFormat::R32SI;
					
				case tdl::DDSFile::DXGIFormat::BC1_UNorm: return ImageFormat::BC1;
				case tdl::DDSFile::DXGIFormat::BC1_UNorm_SRGB: return ImageFormat::BC1SRGB;
					
				case tdl::DDSFile::DXGIFormat::BC2_UNorm: return ImageFormat::BC2;
				case tdl::DDSFile::DXGIFormat::BC2_UNorm_SRGB: return ImageFormat::BC2SRGB;
				
				case tdl::DDSFile::DXGIFormat::BC3_UNorm: return ImageFormat::BC3;
				case tdl::DDSFile::DXGIFormat::BC3_UNorm_SRGB: return ImageFormat::BC3SRGB;
				
				case tdl::DDSFile::DXGIFormat::BC4_UNorm: return ImageFormat::BC4;
				
				case tdl::DDSFile::DXGIFormat::BC5_UNorm: return ImageFormat::BC5;
				
				case tdl::DDSFile::DXGIFormat::BC7_UNorm: return ImageFormat::BC7;
				case tdl::DDSFile::DXGIFormat::BC7_UNorm_SRGB: return ImageFormat::BC7SRGB;
			}

			return ImageFormat::RGBA;
		}
	}

	Ref<Texture2D> DDSTextureImporter::ImportTextureImpl(const std::filesystem::path& path)
	{
		tdl::DDSFile dds;
		auto returnCode = dds.Load(path.string().c_str());
		if (returnCode != tdl::Result::Success)
		{
			Utility::PrintDDSError(returnCode, path);
		}

		if (dds.GetTextureDimension() != tdl::DDSFile::TextureDimension::Texture2D)
		{
			LP_CORE_ERROR("Texture {0} is not 2D!", path.string().c_str());
			return nullptr;
		}

		auto imageData = dds.GetImageData();
		
		VkDeviceSize size = imageData->m_memSlicePitch;
		void* dataPtr = imageData->m_mem;
		uint32_t width = imageData->m_width;
		uint32_t height = imageData->m_height;
		
		VkBuffer stagingBuffer;
		VmaAllocation stagingAllocation;
		
		VulkanAllocator allocator{ "Texture2D - Create" };

		// Create staging buffer
		{
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = size;
			bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			stagingAllocation = allocator.AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);
		}

		// Map memory
		{
			void* bufferPtr = allocator.MapMemory<void>(stagingAllocation);
			memcpy_s(bufferPtr, size, dataPtr, size);
			allocator.UnmapMemory(stagingAllocation);
		}

		Ref<Image2D> image;

		// Create image
		{			
			ImageSpecification imageSpec{};
			imageSpec.format = Utility::DDSToLampImageFormat(dds.GetFormat());
			imageSpec.usage = ImageUsage::Texture;
			imageSpec.width = width;
			imageSpec.height = height;
			imageSpec.mips = dds.GetMipCount();

			image = Image2D::Create(imageSpec);
		
			Utility::TransitionImageLayout(image->GetHandle(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			Utility::CopyBufferToImage(stagingBuffer, image->GetHandle(), width, height);
		}

		allocator.DestroyBuffer(stagingBuffer, stagingAllocation);

		// Set mip data
		{
			for (uint32_t i = 1; i < dds.GetMipCount(); i++)
			{
				VmaAllocation mipStagingAllocation;
				VkBuffer mipStagingBuffer;

				auto mipData = dds.GetImageData(i);
				VkDeviceSize mipSize = mipData->m_memSlicePitch;

				// Create mip staging buffer
				{
					VkBufferCreateInfo bufferInfo{};
					bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
					bufferInfo.size = mipSize;
					bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
					bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

					mipStagingAllocation = allocator.AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, mipStagingBuffer);
				}

				// Map mip memory
				{
					void* bufferPtr = allocator.MapMemory<void>(mipStagingAllocation);
					memcpy_s(bufferPtr, mipSize, mipData->m_mem, mipSize);
					allocator.UnmapMemory(mipStagingAllocation);
				}				
				
				Utility::CopyBufferToImage(mipStagingBuffer, image->GetHandle(), mipData->m_width, mipData->m_height, i);
				allocator.DestroyBuffer(mipStagingBuffer, mipStagingAllocation);
			}

			Utility::TransitionImageLayout(image->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}


		Ref<Texture2D> texture = CreateRef<Texture2D>();
		texture->m_image = image;

		return texture;
	}
}