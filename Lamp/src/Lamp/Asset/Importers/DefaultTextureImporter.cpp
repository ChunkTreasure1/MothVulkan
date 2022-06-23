#include "lppch.h"
#include "DefaultTextureImporter.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/VulkanAllocator.h"

#include "Lamp/Rendering/Texture/Image2D.h"
#include "Lamp/Rendering/Texture/Texture2D.h"

#include "Lamp/Utility/ImageUtility.h"

#include <stb_image/stb_image.h>

namespace Lamp
{
	Ref<Texture2D> DefaultTextureImporter::ImportTextureImpl(const std::filesystem::path& path)
	{
		VkDeviceSize size;
		void* imageData = nullptr;

		int32_t width;
		int32_t height;
		int32_t channels;

		const bool isHDR = stbi_is_hdr(path.string().c_str());

		if (isHDR)
		{
			stbi_set_flip_vertically_on_load(0);
			imageData = stbi_loadf(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
			size = (uint64_t)(width * height * STBI_rgb_alpha * sizeof(float));
		}
		else
		{
			stbi_set_flip_vertically_on_load(1);
			imageData = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
			size = (uint64_t)(width * height * STBI_rgb_alpha);
		}

		VkBuffer stagingBuffer;
		VmaAllocation stagingAllocation;
		auto device = GraphicsContext::GetDevice();

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
			memcpy_s(bufferPtr, size, imageData, size);
			allocator.UnmapMemory(stagingAllocation);
		}

		stbi_image_free(imageData);

		Ref<Image2D> image;

		// Create image
		{
			ImageSpecification imageSpec{};
			imageSpec.format = isHDR ? ImageFormat::RGBA16F : ImageFormat::RGBA;
			imageSpec.usage = ImageUsage::Texture;
			imageSpec.width = (uint32_t)width;
			imageSpec.height = (uint32_t)height;
			imageSpec.mips = (!isHDR) ? static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1 : 1;

			image = Image2D::Create(imageSpec);

			Utility::TransitionImageLayout(image->GetHandle(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			Utility::CopyBufferToImage(stagingBuffer, image->GetHandle(), width, height);
			
			if (!isHDR)
			{
				image->GenerateMips(true);
			}
		}

		allocator.DestroyBuffer(stagingBuffer, stagingAllocation);
		
		Ref<Texture2D> texture = CreateRef<Texture2D>();
		texture->m_image = image;

		return texture;
	}
}