#include "lppch.h"
#include "RenderPassImporter.h"

#include "Lamp/Rendering/Framebuffer.h"
#include "Lamp/Rendering/RenderPass/RenderPass.h"

#include "Lamp/Log/Log.h"

#include "Lamp/Utility/YAMLSerializationHelpers.h"
#include "Lamp/Utility/SerializationMacros.h"
#include <yaml-cpp/yaml.h>

namespace Lamp
{
	namespace Utility
	{
		static ImageFormat FormatFromString(const std::string& string)
		{			 
			if (string == "R32F")
			{
				return ImageFormat::R32F;
			}
			else if (string == "R32SI")
			{
				return ImageFormat::R32SI;
			}
			else if (string == "R32UI")
			{
				return ImageFormat::R32UI;
			}
			else if (string == "RGB")
			{
				return ImageFormat::RGB;
			}
			else if (string == "RGBA")
			{
				return ImageFormat::RGBA;
			}
			else if (string == "RGBA16F")
			{
				return ImageFormat::RGBA16F;
			}
			else if (string == "RGBA32F")
			{
				return ImageFormat::RGBA32F;
			}
			else if (string == "RG16F")
			{
				return ImageFormat::RG16F;
			}
			else if (string == "RG32F")
			{
				return ImageFormat::RG32F;
			}
			else if (string == "SRGB")
			{
				return ImageFormat::SRGB;
			}
			else if (string == "DEPTH32F")
			{
				return ImageFormat::DEPTH32F;
			}
			else if (string == "DEPTH24STENCIL8")
			{
				return ImageFormat::DEPTH24STENCIL8;
			}

			return ImageFormat::None;
		}

		static TextureFilter FilterFromString(const std::string& string)
		{
			if (string == "Linear")
			{
				return TextureFilter::Linear;
			}
			else if (string == "Nearest")
			{
				return TextureFilter::Nearest;
			}

			return TextureFilter::Linear;
		}

		static TextureWrap WrapFromString(const std::string& string)
		{
			if (string == "Clamp")
			{
				return TextureWrap::Clamp;
			}
			else if (string == "Repeat")
			{
				return TextureWrap::Repeat;
			}

			return TextureWrap::Repeat;
		}

		static TextureBlend BlendFromString(const std::string& string)
		{
			if (string == "Min")
			{
				return TextureBlend::Min;
			}
			else if (string == "Max")
			{
				return TextureBlend::Max;
			}

			return TextureBlend::None;
		}

		static ClearMode ClearFromString(const std::string& string)
		{
			if (string == "Clear")
			{
				return ClearMode::Clear;
			}
			else if (string == "Load")
			{
				return ClearMode::Load;
			}
			
			return ClearMode::Clear;
		}

		static DrawType DrawTypeFromString(const std::string& string)
		{
			if (string == "FullscreenQuad")
			{
				return DrawType::FullscreenQuad;
			}
			else if (string == "Opaque")
			{
				return DrawType::Opaque;
			}

			return DrawType::Opaque;
		}
	}

	bool RenderPassImporter::Load(const std::filesystem::path& path, Ref<Asset>& asset) const
	{
		asset = CreateRef<RenderPass>();
		if (!std::filesystem::exists(path)) [[unlikely]]
		{
			LP_CORE_ERROR("File {0} not found!", path.string().c_str());
			asset->SetFlag(AssetFlag::Missing, true);
			return false;
		}

		std::ifstream file(path);
		if (!file.is_open()) [[unlikely]]
		{
			LP_CORE_ERROR("Failed to open file: {0}!", path.string().c_str());
			asset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		std::stringstream sstream;
		sstream << file.rdbuf();
		file.close();

		YAML::Node root = YAML::Load(sstream.str());
		YAML::Node pipelineNode = root["RenderPass"];

		if (!pipelineNode) [[unlikely]]
		{
			LP_CORE_ERROR("Failed to load RenderPipeline from file: {0}!", path.string().c_str());
			asset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		std::string nameString;
		LP_DESERIALIZE_PROPERTY(name, nameString, pipelineNode, std::string());

		std::string overridePipelineString;
		LP_DESERIALIZE_PROPERTY(overridePipeline, overridePipelineString, pipelineNode, std::string());

		std::string drawTypeString;
		LP_DESERIALIZE_PROPERTY(drawType, drawTypeString, pipelineNode, std::string());

		if (!pipelineNode["framebuffer"])
		{
			LP_CORE_ERROR("RenderPass has no framebuffer! This is required!");
			asset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		YAML::Node framebufferNode = pipelineNode["framebuffer"];

		FramebufferSpecification spec{};
		LP_DESERIALIZE_PROPERTY(width, spec.width, framebufferNode, 1280);
		LP_DESERIALIZE_PROPERTY(height, spec.height, framebufferNode, 720);

		if (!framebufferNode["attachments"])
		{
			LP_CORE_ERROR("Framebuffer has no attachments! This is required!");
			asset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		for (const auto& attachment : framebufferNode["attachments"])
		{
			std::string formatString;
			LP_DESERIALIZE_PROPERTY(format, formatString, attachment, std::string());
			
			std::string filterString;
			LP_DESERIALIZE_PROPERTY(filter, filterString, attachment, std::string());

			std::string wrapModeString;
			LP_DESERIALIZE_PROPERTY(wrapMode, wrapModeString, attachment, std::string());

			std::string blendModeString;
			LP_DESERIALIZE_PROPERTY(blendMode, blendModeString, attachment, std::string());
			
			std::string clearModeString;
			LP_DESERIALIZE_PROPERTY(clearMode, clearModeString, attachment, std::string());
			
			glm::vec4 borderColorVal;
			LP_DESERIALIZE_PROPERTY(borderColor, borderColorVal, attachment, glm::vec4(1.f, 1.f, 1.f, 1.f));

			glm::vec4 clearColorVal;
			LP_DESERIALIZE_PROPERTY(clearColor, clearColorVal, attachment, glm::vec4(1.f, 1.f, 1.f, 1.f));
		
			std::string debugNameString;
			LP_DESERIALIZE_PROPERTY(debugName, debugNameString, attachment, std::string());

			auto& framebufferAtt = spec.attachments.emplace_back();
			framebufferAtt.format = Utility::FormatFromString(formatString);
			framebufferAtt.filterMode = Utility::FilterFromString(filterString);
			framebufferAtt.wrapMode = Utility::WrapFromString(wrapModeString);
			framebufferAtt.blendMode = Utility::BlendFromString(blendModeString);
			framebufferAtt.clearMode = Utility::ClearFromString(clearModeString);
			framebufferAtt.borderColor = borderColorVal;
			framebufferAtt.clearColor = clearColorVal;
			framebufferAtt.debugName = debugNameString;
		}

		// TODO: existing images

		Ref<RenderPass> renderPass = CreateRef<RenderPass>();
		renderPass->drawType = Utility::DrawTypeFromString(drawTypeString);
		renderPass->framebuffer = Framebuffer::Create(spec);
		renderPass->name = nameString;
		renderPass->overridePipelineName = overridePipelineString;

		asset = renderPass;
		asset->path = path;

		return true;
	}

	void RenderPassImporter::Save(const Ref<Asset>& asset) const
	{
	}
}