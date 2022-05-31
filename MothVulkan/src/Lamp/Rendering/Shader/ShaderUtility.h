#pragma once

#include <shaderc/shaderc.h>

#include <filesystem>

namespace Lamp
{
	namespace Utility
	{
		inline std::filesystem::path GetShaderCacheDirectory()
		{
			return std::filesystem::path("Engine/Shaders/Cache/");
		}

		inline std::filesystem::path GetShaderDefinitionsDirectory()
		{
			return std::filesystem::path("Engine/Shaders/Definitions");
		}

		inline void CreateCacheDirectoryIfNeeded()
		{
			if (!std::filesystem::exists(GetShaderCacheDirectory()))
			{
				std::filesystem::create_directories(GetShaderCacheDirectory());
			}
		}

		inline void CreateShaderDefDirectoryIfNeeded()
		{
			if (!std::filesystem::exists(GetShaderDefinitionsDirectory()))
			{
				std::filesystem::create_directories(GetShaderDefinitionsDirectory());
			}
		}

		inline std::string ReadStringFromFile(const std::filesystem::path& path)
		{
			std::string result;
			std::ifstream in(path, std::ios::in | std::ios::binary);
			if (in)
			{
				in.seekg(0, std::ios::end);
				result.resize(in.tellg());
				in.seekg(0, std::ios::beg);
				in.read(&result[0], result.size());
			}
			else
			{
				LP_CORE_ASSERT(false, "Unable to read shader!");
			}

			in.close();

			return result;
		}

		inline VkShaderStageFlagBits GetShaderStageFromExtension(const std::string& extension)
		{
			if (extension == ".frag")
			{
				return VK_SHADER_STAGE_FRAGMENT_BIT;
			}
			else if (extension == ".vert")
			{
				return VK_SHADER_STAGE_VERTEX_BIT;
			}
			else if (extension == ".comp")
			{
				return VK_SHADER_STAGE_COMPUTE_BIT;
			}
			else if (extension == ".tessEval")
			{
				return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			}
			else if (extension == ".tessControl")
			{
				return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			}

			return (VkShaderStageFlagBits)0;
		}

		inline std::string GetShaderStageCachedFileExtension(VkShaderStageFlagBits stage)
		{
			switch (stage)
			{
				case VK_SHADER_STAGE_VERTEX_BIT: return ".vertex.cached";
				case VK_SHADER_STAGE_FRAGMENT_BIT: return ".fragment.cached";
				case VK_SHADER_STAGE_COMPUTE_BIT: return ".compute.cached";
				case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return "tessControl.cached";
				case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return "tessEvaluation.cached";
			}

			LP_CORE_ASSERT(false, "Stage not supported!");
			return "";
		}

		inline shaderc_shader_kind VulkanToShaderCStage(VkShaderStageFlagBits stage)
		{
			switch (stage)
			{
				case VK_SHADER_STAGE_VERTEX_BIT: return shaderc_vertex_shader;
				case VK_SHADER_STAGE_FRAGMENT_BIT: return shaderc_fragment_shader;
				case VK_SHADER_STAGE_COMPUTE_BIT: return shaderc_compute_shader;
				case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return shaderc_tess_control_shader;
				case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return shaderc_tess_evaluation_shader;
			}

			LP_CORE_ASSERT(false, "Stage not supported!");
			return (shaderc_shader_kind)0;
		}
	}
}