#include "lppch.h"
#include "ShaderCompiler.h"

#include "ShaderUtility.h"

#include <shaderc/shaderc.hpp>
#include <dxc/dxcapi.h>

#include <file_includer.h>
#include <libshaderc_util/file_finder.h>

#include <combaseapi.h>

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
		fileFinder.search_path().emplace_back("Engine/Shaders/GLSL/");

		compileOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		compileOptions.SetWarningsAsErrors();
		compileOptions.SetIncluder(std::make_unique<glslc::FileIncluder>(&fileFinder));
		compileOptions.SetOptimizationLevel(shaderc_optimization_level_performance);

#ifdef LP_ENABLE_SHADER_DEBUG
		compileOptions.SetGenerateDebugInfo();
#endif

		std::string proccessedSource = src;
		if (!PreprocessGLSL(stage, path, proccessedSource, compiler, compileOptions))
		{
			return false;
		}

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
		if (!DXCInstances::compiler)
		{
			DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&DXCInstances::compiler));
			DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&DXCInstances::utils));
		}

		std::string proccessedSource = src;
		if (!PreprocessHLSL(stage, proccessedSource, proccessedSource))
		{
			return false;
		}

		std::vector<const wchar_t*> arguments
		{ 
			path.c_str(), 
			L"-E", 
			L"main", 
			L"-T", 
			Utility::HLSLShaderProfile(stage), 
			L"-spirv", 
			L"-fspv-target-env=vulkan1.3",
			DXC_ARG_PACK_MATRIX_COLUMN_MAJOR, 
			DXC_ARG_WARNINGS_ARE_ERRORS 
		};

#ifdef LP_ENABLE_SHADER_DEBUG
		arguments.emplace_back(L"-Qembed_debug");
		arguments.emplace_back(DXC_ARG_DEBUG);
#endif

		if (stage & (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_GEOMETRY_BIT))
		{
			arguments.emplace_back(L"-fvk-invert-y");
		}

		IDxcBlobEncoding* sourcePtr;
		DXCInstances::utils->CreateBlob(proccessedSource.c_str(), (uint32_t)proccessedSource.size(), CP_UTF8, &sourcePtr);

		DxcBuffer sourceBuffer{};
		sourceBuffer.Ptr = sourcePtr->GetBufferPointer();
		sourceBuffer.Size = sourcePtr->GetBufferSize();
		sourceBuffer.Encoding = 0;

		IDxcResult* compileResult;
		std::string error;

		HRESULT err = DXCInstances::compiler->Compile(&sourceBuffer, arguments.data(), (uint32_t)arguments.size(), nullptr, IID_PPV_ARGS(&compileResult));

		const bool failed = FAILED(err);
		if (failed)
		{
			error = std::format("Failed to compile. Error: {}\n", err);
			IDxcBlobUtf8* errors;
			compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), NULL);
			if (errors && errors->GetStringLength() > 0)
			{
				error.append(std::format("{}\nWhile compiling shader file: {}", (char*)errors->GetBufferPointer(), path.string()));
			}
		}

		if (error.empty())
		{
			IDxcBlob* result;
			compileResult->GetResult(&result);
			
			const size_t size = result->GetBufferSize();
			outShaderData.resize(size / sizeof(uint32_t));
			memcpy_s(outShaderData.data(), size, result->GetBufferPointer(), size);
			result->Release();
		}

		compileResult->Release();
		sourcePtr->Release();

		return true;
	}

	bool ShaderCompiler::PreprocessGLSL(const VkShaderStageFlagBits stage, const std::filesystem::path& path, std::string& source, shaderc::Compiler& compiler, const shaderc::CompileOptions& compileOptions)
	{
		shaderc::PreprocessedSourceCompilationResult preProcessResult = compiler.PreprocessGlsl(source, Utility::VulkanToShaderCStage(stage), path.string().c_str(), compileOptions);
		if (preProcessResult.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			LP_CORE_ERROR("Failed to preprocess shader {0}!", path.string().c_str());
			LP_CORE_ERROR("{0}", preProcessResult.GetErrorMessage().c_str());

			return false;
		}

		source = std::string(preProcessResult.cbegin(), preProcessResult.cend());
		return true;
	}

	bool ShaderCompiler::PreprocessHLSL(const VkShaderStageFlagBits stage, const std::filesystem::path& path, std::string& source)
	{
		std::vector<const wchar_t*> arguments
		{
			path.c_str(),
			L"-P",
			DXC_ARG_WARNINGS_ARE_ERRORS,
			L"-I Engine/HLSL/"
		};

		IDxcBlobEncoding* sourcePtr;
		DXCInstances::utils->CreateBlob(source.c_str(), (uint32_t)source.size(), CP_UTF8, &sourcePtr);

		DxcBuffer sourceBuffer{};
		sourceBuffer.Ptr = sourcePtr->GetBufferPointer();
		sourceBuffer.Size = sourcePtr->GetBufferSize();
		sourceBuffer.Encoding = 0;

		const Scope<IDxcIncludeHandler> includer = CreateScope<IDxcIncludeHandler>();
		IDxcResult* compileResult;
		HRESULT err = DXCInstances::compiler->Compile(&sourceBuffer, arguments.data(), (uint32_t)arguments.size(), includer.get(), IID_PPV_ARGS(&compileResult));

		std::string error;
		const bool failed = FAILED(err);
		if (failed)
		{
			error = std::format("Failed to compile. Error: {}\n", err);
			IDxcBlobUtf8* errors;
			compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), NULL);
			if (errors && errors->GetStringLength() > 0)
			{
				error.append(std::format("{}\nWhile compiling shader file: {}", (char*)errors->GetBufferPointer(), path.string()));
			}
		}

		if (error.empty())
		{
			IDxcBlob* result;
			compileResult->GetResult(&result);

			source = (const char*)result->GetBufferPointer();
			result->Release();
		}

		sourcePtr->Release();
		compileResult->Release();

		return false;
	}
}