#include "lppch.h"
#include "ShaderCompiler.h"

#include "ShaderUtility.h"

#include <shaderc/shaderc.hpp>
#include <file_includer.h>
#include <libshaderc_util/file_finder.h>

namespace Lamp
{
	namespace Utils
	{
		inline const Shader::Language LanuguageFromPath(const std::filesystem::path& path)
		{
			if (path.extension().string() == ".glsl" || path.extension().string() == ".glslh")
			{
				return Shader::Language::GLSL;
			}
			else if (path.extension().string() == ".hlsl" || path.extension().string() == ".hlslh")
			{
				return Shader::Language::HLSL;
			}

			return Shader::Language::Invalid;
		}
	}

	bool ShaderCompiler::TryCompile(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outShaderData, std::vector<std::filesystem::path> shaderFiles)
	{
		std::unordered_map<VkShaderStageFlagBits, std::string> shaderSources;

		std::vector<Shader::Language> shaderLanguages = GetLanguages(shaderFiles);
		LoadShaderFromFiles(shaderSources, shaderFiles);

		return CompileAll(outShaderData, shaderSources, shaderFiles, shaderLanguages);
	}

	void ShaderCompiler::LoadShaderFromFiles(std::unordered_map<VkShaderStageFlagBits, std::string>& shaderSources, const std::vector<std::filesystem::path>& shaderFiles)
	{
		for (const auto& path : shaderFiles)
		{
			VkShaderStageFlagBits stage = Utility::GetShaderStageFromFilename(path.filename().string());
			std::string source = Utility::ReadStringFromFile(path);

			if (shaderSources.find(stage) != shaderSources.end())
			{
				LP_CORE_ERROR("Multiple shaders of same stage defined in file {0}!", path.string().c_str());
				return;
			}

			shaderSources[stage] = source;
		}
	}

	std::vector<Shader::Language> ShaderCompiler::GetLanguages(const std::vector<std::filesystem::path>& shaderFiles)
	{
		std::vector<Shader::Language> languages;

		for (const auto& path : shaderFiles)
		{
			languages.emplace_back(Utils::LanuguageFromPath(path));
		}

		return languages;
	}

	bool ShaderCompiler::CompileAll(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outShaderData, const std::unordered_map<VkShaderStageFlagBits,
		std::string>& shaderSources, const std::vector<std::filesystem::path>& shaderFiles, const std::vector<Shader::Language>& languages)
	{
		uint32_t index = 0;

		for (const auto& [stage, source] : shaderSources)
		{
			const Shader::Language lang = languages[index];

			if (lang == Shader::Language::GLSL)
			{
				if (!CompileGLSL(stage, source, shaderFiles[index], outShaderData[stage]))
				{
					return false;
				}
			}
			else if (lang == Shader::Language::HLSL)
			{
				if (!CompileHLSL(stage, source, shaderFiles[index], outShaderData[stage]))
				{
					return false;
				}
			}

			index++;
		}

		return true;
	}

	bool ShaderCompiler::CompileGLSL(const VkShaderStageFlagBits stage, const std::string& src, const std::filesystem::path& path, std::vector<uint32_t>& outShaderData)
	{
		auto cacheDirectory = Utility::GetShaderCacheDirectory();
		auto extension = Utility::GetShaderStageCachedFileExtension(stage);

		auto cachedPath = cacheDirectory / (path.filename().string() + extension);

		shaderc::Compiler compiler;
		shaderc::CompileOptions compileOptions;

		shaderc_util::FileFinder fileFinder;
		fileFinder.search_path().emplace_back("Engine/Shaders/");

		compileOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		compileOptions.SetWarningsAsErrors();
		compileOptions.SetIncluder(std::make_unique<glslc::FileIncluder>(&fileFinder));
		compileOptions.SetOptimizationLevel(shaderc_optimization_level_performance);

#ifdef LP_ENABLE_SHADER_DEBUG
		compileOptions.SetGenerateDebugInfo();
#endif

		shaderc::PreprocessedSourceCompilationResult preProcessResult = compiler.PreprocessGlsl(src, Utility::VulkanToShaderCStage(stage), path.string().c_str(), compileOptions);
		if (preProcessResult.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			LP_CORE_ERROR("Failed to preprocess shader {0}!", path.string().c_str());
			LP_CORE_ERROR("{0}", preProcessResult.GetErrorMessage().c_str());

			return false;
		}

		std::string proccessedSource = std::string(preProcessResult.cbegin(), preProcessResult.cend());

		// Compile shader
		{
			shaderc::SpvCompilationResult compileResult = compiler.CompileGlslToSpv(proccessedSource, Utility::VulkanToShaderCStage(stage), path.string().c_str());
			if (compileResult.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				LP_CORE_ERROR("Failed to compile shader {0}!", path.string().c_str());
				LP_CORE_ERROR(compileResult.GetErrorMessage().c_str());

				return false;
			}

			const auto* begin = (const uint8_t*)compileResult.cbegin();
			const auto* end = (const uint8_t*)compileResult.cend();
			const ptrdiff_t size = end - begin;

			outShaderData = std::vector<uint32_t>(compileResult.cbegin(), compileResult.cend());
		}

		// Cache shader
		{
			std::ofstream output(cachedPath, std::ios::binary | std::ios::out);
			if (!output.is_open())
			{
				LP_CORE_ERROR("Failed to open file {0} for writing!", cachedPath.string().c_str());
			}

			output.write((const char*)outShaderData.data(), outShaderData.size() * sizeof(uint32_t));
			output.close();
		}

		return true;
	}

	bool ShaderCompiler::CompileHLSL(const VkShaderStageFlagBits stage, const std::string& src, const std::filesystem::path& path, std::vector<uint32_t>& outShaderData)
	{
		return false;
	}
}