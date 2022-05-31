#pragma once

#include "Lamp/Asset/Asset.h"

#include <vulkan/vulkan.h>

#include <filesystem>

namespace Lamp
{
	class Shader : public Asset
	{
	public:
		Shader(std::initializer_list<std::filesystem::path> paths, bool forceCompile);
		~Shader();

		void Reload(bool forceCompile);
	
		static AssetType GetStaticType() { return AssetType::Shader; }
		AssetType GetType() override { return GetStaticType(); }

		static Ref<Shader> Create(std::initializer_list<std::filesystem::path> paths, bool forceCompile = false);
	
	private:
		void LoadShaderFromFiles();

		void CompileOrGetBinary(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outShaderData, bool forceCompile);

		std::unordered_map<VkShaderStageFlagBits, std::string> m_shaderSources;
		std::vector<std::filesystem::path> m_shaderPaths;
	};
}