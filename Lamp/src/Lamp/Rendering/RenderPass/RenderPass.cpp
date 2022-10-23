#include "lppch.h"
#include "RenderPass.h"

#include "Lamp/Rendering/Shader/ShaderUtility.h"

namespace Lamp
{
	void RenderPass::GenerateHash()
	{
		hash = Utility::HashCombine(hash, std::hash<std::string>()(name));
		hash = Utility::HashCombine(hash, std::hash<std::string>()(overridePipelineName));
		hash = Utility::HashCombine(hash, std::hash<std::string>()(exclusivePipelineName));
		hash = Utility::HashCombine(hash, std::hash<size_t>()(exclusivePipelineHash));
		hash = Utility::HashCombine(hash, std::hash<uint32_t>()((uint32_t)drawType));
		hash = Utility::HashCombine(hash, std::hash<uint32_t>()(priority));
		hash = Utility::HashCombine(hash, std::hash<bool>()(resizeable));
	}
}

