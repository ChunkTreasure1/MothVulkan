#include "lppch.h"
#include "ShaderCompiler.h"

#include "ShaderUtility.h"

#include <shaderc/shaderc.hpp>
#include <file_includer.h>
#include <libshaderc_util/file_finder.h>

namespace Lamp
{
	bool ShaderCompiler::TryCompile(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outShaderData, std::vector<std::filesystem::path> shaderFiles)
	{
		std::unordered_map<VkShaderStageFlagBits, std::string> shaderSources;

		LoadShaderFromFiles(shaderSources, shaderFiles);
		return Compile(outShaderData, shaderSources, shaderFiles);
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

	bool ShaderCompiler::Compile(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outShaderData, const std::unordered_map<VkShaderStageFlagBits, std::string>& shaderSources, const std::vector<std::filesystem::path>& shaderFiles)
	{
		auto cacheDirectory = Utility::GetShaderCacheDirectory();

		uint32_t index = 0;

		for (const auto& [stage, source] : shaderSources)
		{
			auto extension = Utility::GetShaderStageCachedFileExtension(stage);

			auto& data = outShaderData[stage];

			std::filesystem::path currentStagePath;
			for (const auto& shaderPath : shaderFiles)
			{
				if (stage == Utility::GetShaderStageFromFilename(shaderPath.filename().string()))
				{
					currentStagePath = shaderPath;
					break;
				}
			}

			auto cachedPath = cacheDirectory / (currentStagePath.filename().string() + extension);

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

			const auto& currentPath = shaderFiles[index];

			shaderc::PreprocessedSourceCompilationResult preProcessResult = compiler.PreprocessGlsl(source, Utility::VulkanToShaderCStage(stage), currentPath.string().c_str(), compileOptions);
			if (preProcessResult.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				LP_CORE_ERROR("Failed to preprocess shader {0}!", currentPath.string().c_str());
				LP_CORE_ERROR("{0}", preProcessResult.GetErrorMessage().c_str());

				return false;
			}

			std::string proccessedSource = std::string(preProcessResult.cbegin(), preProcessResult.cend());

			// Compile shader
			{
				shaderc::SpvCompilationResult compileResult = compiler.CompileGlslToSpv(proccessedSource, Utility::VulkanToShaderCStage(stage), currentPath.string().c_str());
				if (compileResult.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					LP_CORE_ERROR("Failed to compile shader {0}!", currentPath.string().c_str());
					LP_CORE_ERROR(compileResult.GetErrorMessage().c_str());

					return false;
				}

				const uint8_t* begin = (const uint8_t*)compileResult.cbegin();
				const uint8_t* end = (const uint8_t*)compileResult.cend();
				const ptrdiff_t size = end - begin;

				data = std::vector<uint32_t>(compileResult.cbegin(), compileResult.cend());
			}

			// Cache shader
			{
				std::ofstream output(cachedPath, std::ios::binary | std::ios::out);
				if (!output.is_open())
				{
					LP_CORE_ERROR("Failed to open file {0} for writing!", cachedPath.string().c_str());
				}

				output.write((const char*)data.data(), data.size() * sizeof(uint32_t));
				output.close();
			}

			index++;
		}

		return true;
	}
}