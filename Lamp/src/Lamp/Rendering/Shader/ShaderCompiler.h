#pragma once

#include "Lamp/Rendering/Shader/Shader.h"

#include <vulkan/vulkan_core.h>

namespace Lamp
{
	class ShaderCompiler
	{
	public:
		static bool TryCompile(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outShaderData, std::vector<std::filesystem::path> shaderFiles);

	private:
		static void LoadShaderFromFiles(std::unordered_map<VkShaderStageFlagBits, std::string>& shaderSources, const std::vector<std::filesystem::path>& shaderFiles);
		static std::vector<Shader::Language> GetLanguages(const std::vector<std::filesystem::path>& path);

		static bool CompileAll(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outShaderData, const std::unordered_map<VkShaderStageFlagBits, std::string>& shaderSources, 
			const std::vector<std::filesystem::path>& shaderFiles, const std::vector<Shader::Language>& languages);
	
		static bool CompileGLSL(const VkShaderStageFlagBits stage, const std::string& src, const std::filesystem::path& path, std::vector<uint32_t>& outShaderData);
		static bool CompileHLSL(const VkShaderStageFlagBits stage, const std::string& src, const std::filesystem::path& path, std::vector<uint32_t>& outShaderData);
	};
}