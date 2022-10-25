#include "lppch.h"
#include "ImageBarrier.h"

#include "Lamp/Rendering/Texture/Image2D.h"
#include "Lamp/Utility/ImageUtility.h"

namespace Lamp
{
	ImageBarrier::ImageBarrier(Ref<Image2D> image, VkImageLayout targetLayout)
		: m_image(image)
	{
		m_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		m_barrier.pNext = nullptr;
		m_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		m_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		m_barrier.newLayout = targetLayout;
		m_barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		m_barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		m_barrier.subresourceRange.baseMipLevel = 0;
		m_barrier.subresourceRange.baseArrayLayer = 0;
		m_barrier.subresourceRange.aspectMask = Utility::IsDepthFormat(m_image->GetFormat()) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	}

	const VkImageMemoryBarrier& ImageBarrier::GetBarrier()
	{
		Utility::AccessMasksFromLayouts(m_image->GetLayout(), m_barrier.newLayout, m_barrier);
		m_barrier.image = m_image->GetHandle();
		m_barrier.oldLayout = m_image->GetLayout();

		return m_barrier;
	}

	void ImageBarrier::UpdateLayout(VkImageLayout layout)
	{
		m_image->m_imageLayout = layout;
	}

	Ref<ImageBarrier> ImageBarrier::Create(Ref<Image2D> image, VkImageLayout imageLayout)
	{
		return CreateRef<ImageBarrier>(image, imageLayout);
	}
}