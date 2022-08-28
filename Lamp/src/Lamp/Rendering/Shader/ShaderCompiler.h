#pragma once

#include "Lamp/Rendering/Shader/Shader.h"

#include <vulkan/vulkan.h>


struct IDxcCompiler3;
struct IDxcUtils;

namespace shaderc
{
	class Compiler;
	class CompileOptions;
}

namespace Lamp
{
	class ShaderCompiler
	{
	public:
		static bool TryCompile(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outShaderData, std::vector<std::filesystem::path> shaderFiles);

	private:
		struct DXCInstances
		{
			inline static IDxcCompiler3* compiler = nullptr;
			inline static IDxcUtils* utils = nullptr;
		};

		static void LoadShaderFromFiles(std::unordered_map<VkShaderStageFlagBits, std::string>& shaderSources, const std::vector<std::filesystem::path>& shaderFiles);
		static std::vector<Shader::Language> GetLanguages(const std::vector<std::filesystem::path>& path);

		static bool CompileAll(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outShaderData, const std::unordered_map<VkShaderStageFlagBits, std::string>& shaderSources, 
			const std::vector<std::filesystem::path>& shaderFiles, const std::vector<Shader::Language>& languages);
	
		static bool CompileGLSL(const VkShaderStageFlagBits stage, const std::string& src, const std::filesystem::path& path, std::vector<uint32_t>& outShaderData);
		static bool CompileHLSL(const VkShaderStageFlagBits stage, const std::string& src, const std::filesystem::path& path, std::vector<uint32_t>& outShaderData);

		static bool PreprocessGLSL(const VkShaderStageFlagBits stage, const std::filesystem::path& path, std::string& source, shaderc::Compiler& compiler, const shaderc::CompileOptions& compileOptions);
		static bool PreprocessHLSL(const VkShaderStageFlagBits stage, const std::filesystem::path& path, std::string& source);
	};
}