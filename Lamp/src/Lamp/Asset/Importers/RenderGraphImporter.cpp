#include "lppch.h"
#include "RenderGraphImporter.h"

#include "Lamp/Rendering/RenderGraph.h"
#include "Lamp/Rendering/RenderPass/RenderPassRegistry.h"
#include "Lamp/Log/Log.h"

#include "Lamp/Utility/SerializationMacros.h"
#include "Lamp/Utility/YAMLSerializationHelpers.h"

#include <yaml-cpp/yaml.h>

namespace Lamp
{
	bool RenderGraphImporter::Load(const std::filesystem::path& path, Ref<Asset>& asset) const
	{
		asset = CreateRef<RenderGraph>();
		Ref<RenderGraph> renderGraph = std::reinterpret_pointer_cast<RenderGraph>(asset);

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
		YAML::Node graphNode = root["RenderGraph"];

		if (!graphNode) [[unlikely]]
		{
			LP_CORE_ERROR("Failed to load RenderGraph from file: {0}!", path.string().c_str());
			asset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		std::string nameString;
		LP_DESERIALIZE_PROPERTY(name, nameString, graphNode, std::string("Null"));
		
		YAML::Node renderPassesNode = graphNode["renderPasses"];
		if (renderPassesNode)
		{
			for (const auto& node : renderPassesNode)
			{
				std::string passName;
				LP_DESERIALIZE_PROPERTY(pass, passName, node, std::string("Null"));
				
				int32_t passPriority;
				LP_DESERIALIZE_PROPERTY(priority, passPriority, node, 0);

				Ref<RenderPass> renderPass = RenderPassRegistry::Get(passName);
				if (!renderPass)
				{
					LP_CORE_WARN("Failed to find render pass: {0}! Continuing without it!", passName.c_str());
					continue;
				}

				RenderGraph::RenderPassContainer renderPassCont{};
				renderPassCont.renderPass = renderPass;
				renderPassCont.priority;
			
				renderGraph->m_renderPasses.emplace_back(renderPassCont);
			}
		}

		std::sort(renderGraph->m_renderPasses.begin(), renderGraph->m_renderPasses.end(), [](const RenderGraph::RenderPassContainer& a, const RenderGraph::RenderPassContainer& b) { return a.priority < b.priority; });

		renderGraph->m_name = nameString;
		renderGraph->path = path;

		return true;
	}

	void RenderGraphImporter::Save(const Ref<Asset>& asset) const
	{
	}
}