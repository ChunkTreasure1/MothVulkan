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
}