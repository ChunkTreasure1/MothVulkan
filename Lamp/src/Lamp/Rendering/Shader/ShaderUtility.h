#pragma once

#include "Lamp/Core/Base.h"
#include "Lamp/Log/Log.h"

#include <vulkan/vulkan.h>
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

		inline VkShaderStageFlagBits GetShaderStageFromFilename(const std::string& filename)
		{
			if (filename.find("_fs.glsl") != std::string::npos || filename.find("_ps.hlsl") != std::string::npos)
			{
				return VK_SHADER_STAGE_FRAGMENT_BIT;
			}
			else if (filename.find("_vs.glsl") != std::string::npos || filename.find("_vs.hlsl") != std::string::npos)
			{
				return VK_SHADER_STAGE_VERTEX_BIT;
			}
			else if (filename.find("_cs.glsl") != std::string::npos || filename.find("_cs.hlsl") != std::string::npos)
			{
				return VK_SHADER_STAGE_COMPUTE_BIT;
			}
			else if (filename.find("_te.glsl") != std::string::npos)
			{
				return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			}
			else if (filename.find("_tc.glsl") != std::string::npos)
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

		inline std::string StageToString(VkShaderStageFlagBits stage)
		{
			switch (stage)
			{
				case VK_SHADER_STAGE_VERTEX_BIT: return "Vertex";
				case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return "Tessellation Control";
				case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return "Tessellation Evaluation";
				case VK_SHADER_STAGE_GEOMETRY_BIT: return "Geometry";
				case VK_SHADER_STAGE_FRAGMENT_BIT: return "Fragment";
				case VK_SHADER_STAGE_COMPUTE_BIT: return "Compute";
			}

			return "Unsupported";
		}

		inline static const wchar_t* HLSLShaderProfile(const VkShaderStageFlagBits stage)
		{
			switch (stage)
			{
				case VK_SHADER_STAGE_VERTEX_BIT:    return L"vs_6_0";
				case VK_SHADER_STAGE_FRAGMENT_BIT:  return L"ps_6_0";
				case VK_SHADER_STAGE_COMPUTE_BIT:   return L"cs_6_0";
			}
			LP_CORE_ASSERT(false, "");
			return L"";
		}


		inline size_t HashCombine(size_t lhs, size_t rhs)
		{
			return lhs ^ (rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2));
		}
	}
}