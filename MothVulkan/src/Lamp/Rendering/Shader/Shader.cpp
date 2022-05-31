#include "lppch.h"
#include "Shader.h"

#include "Lamp/Log/Log.h"
#include "Lamp/Rendering/Shader/ShaderUtility.h"

#include <shaderc/shaderc.hpp>
#include <file_includer.h>
#include <libshaderc_util/file_finder.h>

#include <spirv_cross/spirv_glsl.hpp>
#include <spirv-tools/libspirv.h>

namespace Lamp
{
	Shader::Shader(std::initializer_list<std::filesystem::path> paths, bool forceCompile)
		: m_shaderPaths(paths)
	{
		Reload(forceCompile);
	}

	Shader::~Shader()
	{
	}

	void Shader::Reload(bool forceCompile)
	{
		Utility::CreateCacheDirectoryIfNeeded();

		LoadShaderFromFiles();

		std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> shaderData;
		CompileOrGetBinary(shaderData, forceCompile);
	}

	Ref<Shader> Shader::Create(std::initializer_list<std::filesystem::path> paths, bool forceCompile)
	{
		return CreateRef<Shader>(paths, forceCompile);
	}

	void Shader::LoadShaderFromFiles()
	{
		for (const auto& path : m_shaderPaths)
		{
			VkShaderStageFlagBits stage = Utility::GetShaderStageFromExtension(path.extension().string());
			std::string source = Utility::ReadStringFromFile(path);

			if (m_shaderSources.find(stage) != m_shaderSources.end())
			{
				LP_CORE_ERROR("Multiple shaders of same stage defined in file {0}!", path.string().c_str());
				return;
			}

			m_shaderSources[stage] = source;
		}
	}

	void Shader::CompileOrGetBinary(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outShaderData, bool forceCompile)
	{
		auto cacheDirectory = Utility::GetShaderCacheDirectory();

		uint32_t index = 0;

		for (const auto& [stage, source] : m_shaderSources)
		{
			auto extension = Utility::GetShaderStageCachedFileExtension(stage);

			auto& data = outShaderData[stage];

			if (!forceCompile)
			{
				auto cachedPath = cacheDirectory / (path.filename().string() + extension);

				std::ifstream file(cachedPath.string(), std::ios::binary | std::ios::in | std::ios::ate);
				if (file.is_open())
				{
					uint64_t size = file.tellg();

					data.resize(size / sizeof(uint32_t));

					file.seekg(0, std::ios::beg);
					file.read((char*)data.data(), size);
					file.close();
				}

				if (data.empty())
				{
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

					const auto& currentPath = m_shaderPaths[index];

					shaderc::PreprocessedSourceCompilationResult preProcessResult = compiler.PreprocessGlsl(source, Utility::VulkanToShaderCStage(stage), currentPath.string().c_str(), compileOptions);
					if (preProcessResult.GetCompilationStatus() != shaderc_compilation_status_success)
					{
						LP_CORE_ERROR("Failed to preprocess shader {0}!", currentPath.string().c_str());
						LP_CORE_ERROR("{0}", preProcessResult.GetErrorMessage().c_str());
						LP_CORE_ASSERT(false, "Failed to preprocess shader!");
					}

					std::string proccessedSource = std::string(preProcessResult.cbegin(), preProcessResult.cend());

					// Compile shader
					{
						shaderc::SpvCompilationResult compileResult = compiler.CompileGlslToSpv(proccessedSource, Utility::VulkanToShaderCStage(stage), currentPath.string().c_str());
						if (compileResult.GetCompilationStatus() != shaderc_compilation_status_success)
						{
							LP_CORE_ERROR("Failed to compile shader {0}!", currentPath.string().c_str());
							LP_CORE_ERROR(compileResult.GetErrorMessage().c_str());
							LP_CORE_ASSERT(false, "Shader compilation failed!");
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
							LP_CORE_ASSERT(false, "Failed to open file for writing!");
						}

						output.write((const char*)data.data(), data.size() * sizeof(uint32_t));
						output.close();
					}
				}

			}
			
			index++;
		}
	}
}