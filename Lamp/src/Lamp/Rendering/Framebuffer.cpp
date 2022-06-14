#include "lppch.h"
#include "Framebuffer.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"
#include "Lamp/Log/Log.h"

#include "Lamp/Rendering/RenderPipeline/RenderPipeline.h"

#include "Lamp/Utility/ImageUtility.h"

namespace Lamp
{
	namespace Utility
	{
		static VkAttachmentLoadOp LampToVulkanLoadOp(ClearMode clearMode)
		{
			switch (clearMode)
			{
				case ClearMode::Clear: return VK_ATTACHMENT_LOAD_OP_CLEAR;
				case ClearMode::Load: return VK_ATTACHMENT_LOAD_OP_LOAD;
				case ClearMode::DontCare: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			}

			LP_CORE_ASSERT(false, "Clear mode not supported!");
			return VK_ATTACHMENT_LOAD_OP_LOAD;
		}
	}

	Framebuffer::Framebuffer(const FramebufferSpecification& specification)
		: m_specification(specification)
	{
		LP_CORE_ASSERT(!specification.attachments.empty(), "Attachment count must be greater than 0!");
		memset(&m_depthAttachmentInfo, 0, sizeof(VkRenderingAttachmentInfo));

		m_width = m_specification.width;
		m_height = m_specification.height;

		uint32_t attachmentIndex = 0;
		for (auto& attachment : m_specification.attachments)
		{
			if (m_specification.existingImages.find(attachmentIndex) != m_specification.existingImages.end())
			{
				if (!Utility::IsDepthFormat(m_specification.existingImages[attachmentIndex]->GetFormat())) // TODO: should you be able to send in a depth image?
				{
					m_colorAttachmentImages.emplace_back();
				}
			}
			else if (Utility::IsDepthFormat(attachment.format))
			{
				ImageSpecification spec{};
				spec.format = attachment.format;
				spec.usage = ImageUsage::Attachment;
				spec.width = m_width;
				spec.height = m_height;
				spec.copyable = attachment.copyable;

				m_depthAttachmentImage = Image2D::Create(spec);
			}
			else
			{
				ImageSpecification spec{};
				spec.format = attachment.format;
				spec.usage = ImageUsage::Attachment;
				spec.width = m_width;
				spec.height = m_height;
				spec.copyable = attachment.copyable;

				m_colorAttachmentImages.emplace_back(Image2D::Create(spec));
			}

			attachmentIndex++;
		}

		Invalidate();
	}

	Framebuffer::~Framebuffer()
	{
		m_renderPipelineReferences.clear();
		Release();
	}

	void Framebuffer::Invalidate()
	{
		Release();

		auto device = GraphicsContext::GetDevice();
		const bool createImages = m_colorAttachmentImages.empty();

		if (!createImages)
		{
			m_colorFormats.resize(m_colorAttachmentImages.size());
			m_colorAttachmentInfos.resize(m_colorAttachmentImages.size());
		}

		uint32_t attachmentIndex = 0;
		for (auto attachment : m_specification.attachments)
		{
			if (Utility::IsDepthFormat(attachment.format))
			{
				Ref<Image2D> image = m_depthAttachmentImage;
				auto& imageSpec = const_cast<ImageSpecification&>(m_depthAttachmentImage->GetSpecification());
				imageSpec.width = m_width;
				imageSpec.height = m_height;
				imageSpec.copyable = attachment.copyable;

				m_depthAttachmentImage->Invalidate();

				m_depthFormat = Utility::LampToVulkanFormat(m_depthAttachmentImage->GetFormat());
				m_depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				m_depthAttachmentInfo.imageView = m_depthAttachmentImage->GetView();
				m_depthAttachmentInfo.imageLayout = m_depthAttachmentImage->GetFormat() == ImageFormat::DEPTH24STENCIL8 ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
				m_depthAttachmentInfo.loadOp = Utility::LampToVulkanLoadOp(attachment.clearMode);
				m_depthAttachmentInfo.clearValue.depthStencil = { 1.0f, 0 };
			}
			else
			{
				Ref<Image2D> colorAttachment;
				bool existingImage = false;

				if (m_specification.existingImages.find(attachmentIndex) != m_specification.existingImages.end())
				{
					colorAttachment = m_specification.existingImages[attachmentIndex];
					m_colorAttachmentImages[attachmentIndex] = colorAttachment;
					existingImage = true;
				}
				else
				{
					if (createImages)
					{
						ImageSpecification spec{};
						spec.format = attachment.format;
						spec.usage = ImageUsage::Attachment;
						spec.width = m_width;
						spec.height = m_height;
						spec.copyable = attachment.copyable;

						colorAttachment = m_colorAttachmentImages.emplace_back(Image2D::Create(spec));
					}
					else
					{
						Ref<Image2D> image = m_colorAttachmentImages[attachmentIndex];
						ImageSpecification& spec = const_cast<ImageSpecification&>(image->GetSpecification());

						spec.width = m_width;
						spec.height = m_height;
						spec.copyable = attachment.copyable;

						colorAttachment = image;
						colorAttachment->Invalidate(nullptr);
					}
				}

				m_colorFormats[attachmentIndex] = Utility::LampToVulkanFormat(colorAttachment->GetFormat());

				auto& attInfo = m_colorAttachmentInfos[attachmentIndex];
				attInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				attInfo.imageView = colorAttachment->GetView();
				attInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				attInfo.loadOp = existingImage ? VK_ATTACHMENT_LOAD_OP_LOAD : Utility::LampToVulkanLoadOp(attachment.clearMode);
				attInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				attInfo.clearValue = { attachment.clearColor.x, attachment.clearColor.y, attachment.clearColor.z, attachment.clearColor.w };
			}

			attachmentIndex++;
		}

		std::vector<VkImageView> attachments(m_colorAttachmentImages.size());
		for (uint32_t i = 0; i < m_colorAttachmentImages.size(); i++)
		{
			attachments[i] = m_colorAttachmentImages[i]->GetView();
			LP_CORE_ASSERT(attachments[i], "Image view was nullptr!");
		}

		if (m_depthAttachmentImage)
		{
			attachments.emplace_back(m_depthAttachmentImage->GetView());
			LP_CORE_ASSERT(attachments.back(), "Image view was nullptr!");
		}

		for (const auto& pipeline : m_renderPipelineReferences)
		{
			pipeline->InvalidateMaterials();
		}
	}

	void Framebuffer::Bind(VkCommandBuffer cmdBuffer) const
	{
		for (auto& attachment : m_colorAttachmentImages)
		{
			const VkImageLayout oldLayout = m_firstBind ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			Utility::InsertImageMemoryBarrier(cmdBuffer, attachment->GetHandle(), 0,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				oldLayout,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		}

		if (m_depthAttachmentImage)
		{
			const VkImageLayout oldLayout = m_firstBind ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

			Utility::InsertImageMemoryBarrier(cmdBuffer, m_depthAttachmentImage->GetHandle(), 0,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				oldLayout,
				VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 });
		}

		m_firstBind = false;
	}

	void Framebuffer::Unbind(VkCommandBuffer cmdBuffer) const
	{
		for (auto& attachment : m_colorAttachmentImages)
		{
			Utility::InsertImageMemoryBarrier(cmdBuffer, attachment->GetHandle(),
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				0,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		}

		if (m_depthAttachmentImage)
		{
			Utility::InsertImageMemoryBarrier(cmdBuffer, m_depthAttachmentImage->GetHandle(),
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				0,
				VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 });
		}
	}

	void Framebuffer::Resize(uint32_t width, uint32_t height)
	{
		m_width = width;
		m_height = height;
		m_firstBind = true;

		Invalidate();
	}

	void Framebuffer::AddReference(RenderPipeline* renderPipeline)
	{
		if (auto it = std::find(m_renderPipelineReferences.begin(), m_renderPipelineReferences.end(), renderPipeline); it != m_renderPipelineReferences.end())
		{
			LP_CORE_ERROR("Framebuffer already has a reference to render pipeline {0}", renderPipeline->GetSpecification().name.c_str());
			return;
		}

		m_renderPipelineReferences.emplace_back(renderPipeline);
	}

	void Framebuffer::RemoveReference(RenderPipeline* renderPipeline)
	{
		auto it = std::find(m_renderPipelineReferences.begin(), m_renderPipelineReferences.end(), renderPipeline);
		if (it == m_renderPipelineReferences.end())
		{
			LP_CORE_ERROR("Reference to render pipeline {0} not found in framebuffer!", renderPipeline->GetSpecification().name.c_str());
			return;
		}

		m_renderPipelineReferences.erase(it);
	}

	Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& specification)
	{
		return CreateRef<Framebuffer>(specification);
	}

	void Framebuffer::Release()
	{
		uint32_t attachmentIndex = 0;

		for (auto image : m_colorAttachmentImages)
		{
			if (m_specification.existingImages.find(attachmentIndex) != m_specification.existingImages.end())
			{
				continue;
			}

			image->Release();

			attachmentIndex++;
		}

		if (m_depthAttachmentImage)
		{
			m_depthAttachmentImage->Release();
		}

		m_colorFormats.clear();
		m_colorAttachmentInfos.clear();
	}
}