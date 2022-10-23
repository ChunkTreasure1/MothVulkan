#include "lppch.h"
#include "RenderPassImporter.h"

#include "Lamp/Rendering/Framebuffer.h"
#include "Lamp/Rendering/RenderPass/RenderPass.h"

#include "Lamp/Rendering/RenderPipeline/RenderPipelineRegistry.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipeline.h"

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

		static std::string StringFromFormat(ImageFormat format)
		{
			switch (format)
			{
			case ImageFormat::None: return "None";
			case ImageFormat::R32F: return "R32F";
			case ImageFormat::R32SI: return "R32SI";
			case ImageFormat::R32UI: return "R32UI";
			case ImageFormat::RGB: return "RGB";
			case ImageFormat::RGBA: return "RGBA";
			case ImageFormat::RGBA16F: return "RGBA16F";
			case ImageFormat::RGBA32F: return "RGBA32F";
			case ImageFormat::RG16F: return "RG16F";
			case ImageFormat::RG32F: return "RG32F";
			case ImageFormat::SRGB: return "SRGB";
			case ImageFormat::BC1: return "BC1";
			case ImageFormat::BC1SRGB: return "BC1SRGB";
			case ImageFormat::BC2: return "BC2";
			case ImageFormat::BC2SRGB: return "BC2SRGB";
			case ImageFormat::BC3: return "BC3";
			case ImageFormat::BC3SRGB: return "BC3SRGB";
			case ImageFormat::BC4: return "BC4";
			case ImageFormat::BC7: return "BC7";
			case ImageFormat::BC7SRGB: return "BC7SRGB";
			case ImageFormat::DEPTH32F: return "DEPTH32F";
			case ImageFormat::DEPTH24STENCIL8: return "DEPTH24STENCIL8";
			}

			return "";
		}

		static std::string StringFromFilter(TextureFilter filter)
		{
			switch (filter)
			{
			case TextureFilter::None: return "None";
			case TextureFilter::Linear: return "Linear";
			case TextureFilter::Nearest: return "Nearest";
			}

			return "";
		}

		static std::string StringFromWrap(TextureWrap wrap)
		{
			switch (wrap)
			{
			case TextureWrap::None: return "None";
			case TextureWrap::Clamp: return "Clamp";
			case TextureWrap::Repeat: return "Repeat";
			}

			return "";
		}

		static std::string StringFromBlend(TextureBlend blend)
		{
			switch (blend)
			{
			case TextureBlend::None: return "None";
			case TextureBlend::Min: return "Min";
			case TextureBlend::Max: return "Max";
			}

			return "";
		}

		static std::string StringFromClearMode(ClearMode clearMode)
		{
			switch (clearMode)
			{
			case ClearMode::Clear: return "Clear";
			case ClearMode::Load: return "Load";
			case ClearMode::DontCare: return "DontCare";
			}

			return "";
		}

		static std::string StringFromDrawType(DrawType type)
		{
			switch (type)
			{
			case DrawType::FullscreenQuad: return "FullscreenQuad";
			case DrawType::Opaque: return "Opaque";
			}

			return "";
		}
	}

	bool RenderPassImporter::Load(const std::filesystem::path& path, Ref<Asset>& asset) const
	{
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

		std::string exclusivePipelineName;
		LP_DESERIALIZE_PROPERTY(exclusivePipeline, exclusivePipelineName, pipelineNode, std::string());

		std::string computePipelineString;
		LP_DESERIALIZE_PROPERTY(computePipeline, computePipelineString, pipelineNode, std::string());

		std::string drawTypeString;
		LP_DESERIALIZE_PROPERTY(drawType, drawTypeString, pipelineNode, std::string());

		int32_t priority;
		LP_DESERIALIZE_PROPERTY(priority, priority, pipelineNode, 0);

		bool resizeable;
		LP_DESERIALIZE_PROPERTY(resizeable, resizeable, pipelineNode, true);

		std::vector<std::string> excludedPipelines;

		if (pipelineNode["excludedPipelines"])
		{
			const YAML::Node excludedNode = pipelineNode["excludedPipelines"];
			for (const auto& pipeline : excludedNode)
			{
				std::string pipelineName;
				LP_DESERIALIZE_PROPERTY(name, pipelineName, pipeline, std::string());
				excludedPipelines.emplace_back(pipelineName);
			}
		}

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

		std::vector<RenderPass::ExistingImage> existingImages;

		for (const auto& imageNode : framebufferNode["existingImages"])
		{
			auto& image = existingImages.emplace_back();

			LP_DESERIALIZE_PROPERTY(renderPass, image.renderPass, imageNode, std::string());
			LP_DESERIALIZE_PROPERTY(attachmentIndex, image.attachmentIndex, imageNode, 0);
			LP_DESERIALIZE_PROPERTY(targetIndex, image.targetIndex, imageNode, 0);
			LP_DESERIALIZE_PROPERTY(depth, image.depth, imageNode, false);
		}

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

			bool copyableVal;
			LP_DESERIALIZE_PROPERTY(copyable, copyableVal, attachment, false);

			auto& framebufferAtt = spec.attachments.emplace_back();
			framebufferAtt.format = Utility::FormatFromString(formatString);
			framebufferAtt.filterMode = Utility::FilterFromString(filterString);
			framebufferAtt.wrapMode = Utility::WrapFromString(wrapModeString);
			framebufferAtt.blendMode = Utility::BlendFromString(blendModeString);
			framebufferAtt.clearMode = Utility::ClearFromString(clearModeString);
			framebufferAtt.borderColor = borderColorVal;
			framebufferAtt.clearColor = clearColorVal;
			framebufferAtt.debugName = debugNameString;
			framebufferAtt.copyable = copyableVal;
		}

		// TODO: existing images

		Ref<RenderPass> renderPass = CreateRef<RenderPass>();
		renderPass->drawType = Utility::DrawTypeFromString(drawTypeString);
		renderPass->framebuffer = Framebuffer::Create(spec);
		renderPass->name = nameString;
		renderPass->overridePipelineName = overridePipelineString;
		renderPass->priority = priority;
		renderPass->excludedPipelineNames = excludedPipelines;
		renderPass->computePipelineName = computePipelineString;
		renderPass->existingImages = existingImages;

		if (!exclusivePipelineName.empty())
		{
			renderPass->exclusivePipelineName = exclusivePipelineName;
		}

		asset = renderPass;
		asset->path = path;

		return true;
	}

	void RenderPassImporter::Save(const Ref<Asset>& asset) const
	{
		Ref<RenderPass> renderPass = std::reinterpret_pointer_cast<RenderPass>(asset);

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "RenderPass" << YAML::Value;
		out << YAML::BeginMap;
		{
			LP_SERIALIZE_PROPERTY(name, renderPass->name, out);

			if (renderPass->overridePipeline)
			{
				LP_SERIALIZE_PROPERTY(overridePipeline, renderPass->overridePipelineName, out);
			}

			if (renderPass->computePipeline)
			{
				LP_SERIALIZE_PROPERTY(computePipeline, renderPass->computePipelineName, out);
			}

			if (renderPass->exclusivePipelineHash != 0)
			{
				LP_SERIALIZE_PROPERTY(exclusivePipelineName, renderPass->exclusivePipelineName, out);
			}

			LP_SERIALIZE_PROPERTY(drawType, Utility::StringFromDrawType(renderPass->drawType), out);
			LP_SERIALIZE_PROPERTY(priority, renderPass->priority, out);
			LP_SERIALIZE_PROPERTY(resizeable, renderPass->resizeable, out);

			out << YAML::Key << "framebuffer" << YAML::Value;
			out << YAML::BeginMap;
			auto framebuffer = renderPass->framebuffer;
			if (framebuffer)
			{
				LP_SERIALIZE_PROPERTY(width, framebuffer->GetSpecification().width, out);
				LP_SERIALIZE_PROPERTY(height, framebuffer->GetSpecification().height, out);

				out << YAML::Key << "attachments" << YAML::BeginSeq;
				for (const auto& attachment : framebuffer->GetSpecification().attachments)
				{
					out << YAML::BeginMap;
					LP_SERIALIZE_PROPERTY(format, Utility::StringFromFormat(attachment.format), out);
					LP_SERIALIZE_PROPERTY(filter, Utility::StringFromFilter(attachment.filterMode), out);
					LP_SERIALIZE_PROPERTY(wrapMode, Utility::StringFromWrap(attachment.wrapMode), out);
					LP_SERIALIZE_PROPERTY(blendMode, Utility::StringFromBlend(attachment.blendMode), out);
					LP_SERIALIZE_PROPERTY(clearMode, Utility::StringFromClearMode(attachment.clearMode), out);
					LP_SERIALIZE_PROPERTY(borderColor, attachment.borderColor, out);
					LP_SERIALIZE_PROPERTY(clearColor, attachment.clearColor, out);
					LP_SERIALIZE_PROPERTY(debugName, attachment.debugName, out);
					LP_SERIALIZE_PROPERTY(copyable, attachment.copyable, out);
					out << YAML::EndMap;
				}
				out << YAML::EndSeq;
			}
			out << YAML::EndMap;

		}
		out << YAML::EndMap;
		out << YAML::EndMap;

		std::ofstream fout(asset->path);
		fout << out.c_str();
		fout.close();
	}
}