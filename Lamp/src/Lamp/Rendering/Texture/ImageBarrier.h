#pragma once

#include "Lamp/Core/Base.h"

#include "vulkan/vulkan.h"

namespace Lamp
{
	class Image2D;
	class ImageBarrier
	{
	public:
		ImageBarrier(Ref<Image2D> image, VkImageLayout targetLayout);
		
		const VkImageMemoryBarrier& GetBarrier();
		void UpdateLayout(VkImageLayout layout);

		static Ref<ImageBarrier> Create(Ref<Image2D> image, VkImageLayout targetLayout);

	private:
		VkImageMemoryBarrier m_barrier;
		Ref<Image2D> m_image;
	};
}