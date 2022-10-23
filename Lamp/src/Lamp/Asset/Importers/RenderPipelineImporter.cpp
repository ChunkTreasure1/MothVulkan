#include "lppch.h"
#include "RenderPipelineImporter.h"

#include "Lamp/Log/Log.h"

#include "Lamp/Rendering/RenderPipeline/RenderPipeline.h"
#include "Lamp/Rendering/Vertex.h"
#include "Lamp/Rendering/Shader/ShaderRegistry.h"

#include "Lamp/Utility/YAMLSerializationHelpers.h"
#include "Lamp/Utility/SerializationMacros.h"

#include "Lamp/Rendering/RenderPass/RenderPassRegistry.h"
#include "Lamp/Rendering/RenderPass/RenderPass.h"
#include "Lamp/Rendering/Shader/Shader.h"

namespace Lamp
{
	namespace Utility
	{
		inline Topology TopologyFromString(const std::string& string)
		{
			if (string == "TriangleList")
			{
				return Topology::TriangleList;
			}
			else if (string == "TriangleStrip")
			{
				return Topology::TriangleStrip;
			}
			else if (string == "LineList")
			{
				return Topology::LineList;
			}
			else if (string == "PatchList")
			{
				return Topology::PatchList;
			}

			return Topology::TriangleList;
		}

		inline CullMode CullModeFromString(const std::string& string)
		{
			if (string == "Back")
			{
				return CullMode::Back;
			}
			else if (string == "Front")
			{
				return CullMode::Front;
			}
			else if (string == "FrontAndBack")
			{
				return CullMode::FrontAndBack;
			}
			else if (string == "None")
			{
				return CullMode::None;
			}

			return CullMode::Back;
		}

		inline FillMode FillModeFromString(const std::string& string)
		{
			if (string == "Solid")
			{
				return FillMode::Solid;
			}
			else if (string == "Wireframe")
			{
				return FillMode::Wireframe;
			}

			return FillMode::Solid;
		}

		inline DepthMode DepthModeFromString(const std::string& string)
		{
			if (string == "Read")
			{
				return DepthMode::Read;
			}
			else if (string == "Write")
			{
				return DepthMode::Write;
			}
			else if (string == "ReadWrite")
			{
				return DepthMode::ReadWrite;
			}

			return DepthMode::ReadWrite;
		}

		inline std::string StringFromTopology(Topology topology)
		{
			switch (topology)
			{
				case Topology::LineList: return "LineList";
				case Topology::PatchList: return "PatchList";
				case Topology::TriangleList: return "TriangleList";
				case Topology::TriangleStrip: return "TriangleStrip";
			}

			return "";
		}

		inline std::string StringFromCullMode(CullMode cullMode)
		{
			switch (cullMode)
			{
				case CullMode::Front: return "Front";
				case CullMode::Back: return "Back";
				case CullMode::FrontAndBack: return "FrontAndBack";
				case CullMode::None: return "None";
			}

			return "";
		}

		inline std::string StringFromFillMode(FillMode fillMode)
		{
			switch (fillMode)
			{
				case FillMode::Wireframe: return "Wireframe";
				case FillMode::Solid: return "Solid";
			}

			return "";
		}

		inline std::string StringFromDepthMode(DepthMode depthMode)
		{
			switch (depthMode)
			{
				case DepthMode::Read: return "Read";
				case DepthMode::Write: return "Write";
				case DepthMode::ReadWrite: return "ReadWrite";
				case DepthMode::None: return "None";
			}

			return "";
		}
	}

	bool RenderPipelineImporter::Load(const std::filesystem::path& path, Ref<Asset>& asset) const
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
		YAML::Node pipelineNode = root["RenderPipeline"];

		if (!pipelineNode) [[unlikely]]
		{
			LP_CORE_ERROR("Failed to load RenderPipeline from file: {0}!", path.string().c_str());
			asset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		RenderPipelineSpecification pipelineSpec{};
		LP_DESERIALIZE_PROPERTY(name, pipelineSpec.name, pipelineNode, std::string());

		// Data
		{
			std::string shaderName;
			LP_DESERIALIZE_PROPERTY(shader, shaderName, pipelineNode, std::string());
			pipelineSpec.shader = ShaderRegistry::Get(shaderName);

			std::string renderPassName;
			LP_DESERIALIZE_PROPERTY(renderPass, renderPassName, pipelineNode, std::string());
			
			Ref<Framebuffer> framebuffer;

			if (auto renderPass = RenderPassRegistry::Get(renderPassName))
			{
				framebuffer = renderPass->framebuffer;
			}
			
			if (!framebuffer)
			{
				LP_CORE_ERROR("Invalid renderpass: {0}!", path.string().c_str());
				asset->SetFlag(AssetFlag::Invalid, true);
				return false;
			}
			pipelineSpec.renderPass = renderPassName;
			pipelineSpec.framebuffer =  framebuffer;

			std::string topologyName;
			LP_DESERIALIZE_PROPERTY(topology, topologyName, pipelineNode, std::string());
			pipelineSpec.topology = Utility::TopologyFromString(topologyName);

			std::string cullModeName;
			LP_DESERIALIZE_PROPERTY(cullMode, cullModeName, pipelineNode, std::string());
			pipelineSpec.cullMode = Utility::CullModeFromString(cullModeName);

			std::string fillModeName;
			LP_DESERIALIZE_PROPERTY(fillMode, fillModeName, pipelineNode, std::string());
			pipelineSpec.fillMode = Utility::FillModeFromString(fillModeName);

			std::string depthModeName;
			LP_DESERIALIZE_PROPERTY(depthMode, depthModeName, pipelineNode, std::string());
			pipelineSpec.depthMode = Utility::DepthModeFromString(depthModeName);

			LP_DESERIALIZE_PROPERTY(depthTest, pipelineSpec.depthTest, pipelineNode, true);
			LP_DESERIALIZE_PROPERTY(depthWrite, pipelineSpec.depthWrite, pipelineNode, true);
			LP_DESERIALIZE_PROPERTY(lineWidth, pipelineSpec.lineWidth, pipelineNode, 1.f);
			LP_DESERIALIZE_PROPERTY(tessellationControlPoints, pipelineSpec.tessellationControlPoints, pipelineNode, 4);

			if (pipelineNode["framebufferInputs"])
			{
				std::vector<FramebufferInput> framebufferInputs;
				for (const auto& node : pipelineNode["framebufferInputs"])
				{
					std::string renderPassInputName;
					LP_DESERIALIZE_PROPERTY(renderPass, renderPassInputName, node, std::string());
				
					uint32_t attachmentIndex;
					LP_DESERIALIZE_PROPERTY(attachment, attachmentIndex, node, 0);

					uint32_t shaderSet;
					LP_DESERIALIZE_PROPERTY(set, shaderSet, node, 0);

					uint32_t shaderBinding;
					LP_DESERIALIZE_PROPERTY(binding, shaderBinding, node, 0);

					if (renderPassInputName.empty())
					{
						LP_CORE_ERROR("Render pass was empty! This is not valid! Setting default texture!");
						continue;
					}

					Ref<RenderPass> renderPass = RenderPassRegistry::Get(renderPassInputName);
					if (!renderPass)
					{
						LP_CORE_ERROR("Render pass with name {0} not found! Using default texture!", renderPassInputName.c_str());
						continue;
					}
					
					framebufferInputs.emplace_back(FramebufferInput{ renderPass->framebuffer, renderPassInputName, attachmentIndex, shaderSet, shaderBinding });
				}

				pipelineSpec.framebufferInputs = framebufferInputs;
			}

			if (pipelineNode["vertexLayout"])
			{
				std::vector<BufferElement> bufferElements;
				
				YAML::Node bufferLayoutNode = pipelineNode["vertexLayout"];
				for (const auto& element : bufferLayoutNode)
				{
					std::string typeName;
					LP_DESERIALIZE_PROPERTY(type, typeName, element, std::string());

					std::string elementName;
					LP_DESERIALIZE_PROPERTY(name, elementName, element, std::string());

					bufferElements.emplace_back(BufferElement::GetTypeFromString(typeName), elementName);
				}

				pipelineSpec.vertexLayout = BufferLayout(bufferElements);
			}
			else
			{
				pipelineSpec.vertexLayout = Vertex::GetLayout();
			}

			if (pipelineNode["instanceLayout"])
			{
				std::vector<BufferElement> bufferElements;

				YAML::Node bufferLayoutNode = pipelineNode["instanceLayout"];
				for (const auto& element : bufferLayoutNode)
				{
					std::string typeName;
					LP_DESERIALIZE_PROPERTY(type, typeName, element, std::string());

					std::string elementName;
					LP_DESERIALIZE_PROPERTY(name, elementName, element, std::string());

					bufferElements.emplace_back(BufferElement::GetTypeFromString(typeName), elementName);
				}

				pipelineSpec.instanceLayout = BufferLayout(bufferElements);
			}
		}

		Ref<RenderPipeline> renderPipeline = RenderPipeline::Create(pipelineSpec);
		if (!renderPipeline)
		{
			LP_CORE_ERROR("Unable to create pipeline {0}!", path.string().c_str());
			asset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		asset = renderPipeline;
		asset->path = path;

		return true;
	}

	void RenderPipelineImporter::Save(const Ref<Asset>& asset) const
	{
		Ref<RenderPipeline> pipeline = std::reinterpret_pointer_cast<RenderPipeline>(asset);
		
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "RenderPipeline" << YAML::Value;
		{
			out << YAML::BeginMap;
			LP_SERIALIZE_PROPERTY(name, pipeline->GetSpecification().name, out);
			LP_SERIALIZE_PROPERTY(shader, pipeline->GetSpecification().shader->GetName(), out);
			LP_SERIALIZE_PROPERTY(renderPass, pipeline->GetSpecification().renderPass, out);
			LP_SERIALIZE_PROPERTY(topology, Utility::StringFromTopology(pipeline->GetSpecification().topology), out);
			LP_SERIALIZE_PROPERTY(cullMode, Utility::StringFromCullMode(pipeline->GetSpecification().cullMode), out);
			LP_SERIALIZE_PROPERTY(depthMode, Utility::StringFromDepthMode(pipeline->GetSpecification().depthMode), out);
			LP_SERIALIZE_PROPERTY(depthTest, pipeline->GetSpecification().depthTest, out);
			LP_SERIALIZE_PROPERTY(depthWrite, pipeline->GetSpecification().depthWrite, out);
			LP_SERIALIZE_PROPERTY(lineWidth, pipeline->GetSpecification().lineWidth, out);
			LP_SERIALIZE_PROPERTY(tessellationControlPoints, pipeline->GetSpecification().tessellationControlPoints, out);

			// Vertex Layout
			{
				out << YAML::Key << "vertexLayout" << YAML::BeginSeq;
				for (const auto& element : pipeline->GetSpecification().vertexLayout.GetElements())
				{
					out << YAML::BeginMap;
					LP_SERIALIZE_PROPERTY(type, BufferElement::GetStringFromElementType(element.type), out);
					LP_SERIALIZE_PROPERTY(name, element.name, out);
					out << YAML::EndMap;
				}
				out << YAML::EndSeq;
			}

			// Instance Layout
			{
				out << YAML::Key << "instanceLayout" << YAML::BeginSeq;
				for (const auto& element : pipeline->GetSpecification().instanceLayout.GetElements())
				{
					out << YAML::BeginMap;
					LP_SERIALIZE_PROPERTY(type, BufferElement::GetStringFromElementType(element.type), out);
					LP_SERIALIZE_PROPERTY(name, element.name, out);
					out << YAML::EndMap;
				}
				out << YAML::EndSeq;
			}

			// Framebuffer Inputs
			{
				out << YAML::Key << "framebufferInputs" << YAML::BeginSeq;
				for (const auto& input : pipeline->GetSpecification().framebufferInputs)
				{
					out << YAML::BeginMap;
					LP_SERIALIZE_PROPERTY(renderPass, input.renderPass, out);
					LP_SERIALIZE_PROPERTY(attachment, input.attachmentIndex, out);
					LP_SERIALIZE_PROPERTY(set, input.set, out);
					LP_SERIALIZE_PROPERTY(binding, input.binding, out);
					out << YAML::EndMap;
				}
				out << YAML::EndSeq;
			}

			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		std::ofstream fout(asset->path);
		fout << out.c_str();
		fout.close();
	}
}