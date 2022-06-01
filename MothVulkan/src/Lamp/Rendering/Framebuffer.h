#pragma once

#include "Lamp/Rendering/Texture/Image2D.h"

namespace Lamp
{
	class Image2D;

	enum class ClearMode : uint32_t
	{
		Clear,
		Load,
		DontCare
	};

	struct FramebufferAttachment
	{
		FramebufferAttachment(ImageFormat aFormat, ClearMode aClearMode)
			: format(aFormat), clearMode(aClearMode)
		{}

		FramebufferAttachment(ImageFormat aFormat)
			: format(aFormat)
		{}

		FramebufferAttachment(ImageFormat aFormat, TextureFilter aFilter, 
			TextureWrap aWrap, TextureBlend aBlend, ClearMode aClearMode,
			const glm::vec4& aBorderColor = { 1.f, 1.f, 1.f, 1.f }, 
			const glm::vec4 aClearColor = { 1.f, 1.f, 1.f, 1.f }, const std::string& aDebugName = "")
			
			: format(aFormat), filterMode(aFilter), wrapMode(aWrap), blendMode(aBlend), clearMode(aClearMode),
			borderColor(aBorderColor), clearColor(aClearColor), debugName(aDebugName)
		{ }

		ImageFormat format = ImageFormat::RGBA;
		TextureFilter filterMode = TextureFilter::Linear;
		TextureWrap wrapMode = TextureWrap::Repeat;
		TextureBlend blendMode = TextureBlend::None;
		ClearMode clearMode = ClearMode::Clear;

		glm::vec4 borderColor = { 1.f, 1.f, 1.f, 1.f };
		glm::vec4 clearColor = { 1.f, 1.f, 1.f, 1.f };
		
		std::string debugName;
	};

	struct FramebufferSpecification
	{
		uint32_t width = 1280;
		uint32_t height = 720;
	
		std::vector<FramebufferAttachment> attachments;
		std::map<uint32_t, Ref<Image2D>> existingImages;
	};

	class Framebuffer
	{
	public:
		Framebuffer(const FramebufferSpecification& specification);
		~Framebuffer();

		void Invalidate();

		void Bind(VkCommandBuffer cmdBuffer) const;
		void Unbind(VkCommandBuffer cmdBuffer) const;
		
		void Resize(uint32_t width, uint32_t height);

		inline const Ref<Image2D> GetDepthAttachment() const { return m_depthAttachmentImage; }
		inline const Ref<Image2D> GetColorAttachment(uint32_t index) const { return m_colorAttachmentImages[index]; }
		inline const FramebufferSpecification& GetSpecification() const { return m_specification; }

		inline const std::vector<VkRenderingAttachmentInfo>& GetColorAttachmentInfos() const { return m_colorAttachmentInfos; }
		inline const VkRenderingAttachmentInfo& GetDepthAttachmentInfo() const { return m_depthAttachmentInfo; }

		inline const uint32_t GetWidth() const { return m_width; }
		inline const uint32_t GetHeight() const { return m_height; }
		
		static Ref<Framebuffer> Create(const FramebufferSpecification& specification);

	private:
		friend class RenderPipeline;

		void Release();

		FramebufferSpecification m_specification;

		mutable bool m_firstBind = true;

		uint32_t m_width;
		uint32_t m_height;
		
		Ref<Image2D> m_depthAttachmentImage;
		std::vector<Ref<Image2D>> m_colorAttachmentImages;

		std::vector<VkFormat> m_colorFormats;
		VkFormat m_depthFormat;

		std::vector<VkRenderingAttachmentInfo> m_colorAttachmentInfos;
		VkRenderingAttachmentInfo m_depthAttachmentInfo;
	};
}