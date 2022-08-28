
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Sandbox"
	location "."
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++latest"
	debugdir "../Resources"

	targetdir ("../bin/" .. outputdir .."/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .."/%{prj.name}")

	pchheader "sbpch.h"
	pchsource "src/sbpch.cpp"

	disablewarnings
	{
		"4005"
	}

	linkoptions 
	{
		"/ignore:4006",
		"/ignore:4099",
		"/ignore:4098",
		"/WHOLEARCHIVE:Lamp"
	}

    defines
    {
        "GLFW_INCLUDE_NONE",
		"GLM_FORCE_DEPTH_ZERO_TO_ONE",
		"GLM_FORCE_SSE2",
		"NOMINMAX"
    }

	files
	{
		"src/**.h",
		"src/**.cpp",
		"src/**.hpp",
	}

	includedirs
	{
		"src/",
		"../Lamp/src/",

        "%{IncludeDir.VulkanSDK}",
        "%{IncludeDir.GLFW}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.yaml}",
		"%{IncludeDir.fbxsdk}",
		"%{IncludeDir.stb}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.Wire}",
		"%{IncludeDir.vma}",
		"%{IncludeDir.Optick}",
		"%{IncludeDir.TinyGLTF}",
		"%{IncludeDir.imgui_notify}",
		"%{IncludeDir.imgui_node_editor}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.P4}",
	}

    links
    {
        "Lamp",

		"GLFW",
		"ImGui",
		"Wire",
		"Optick",
		"ImGuiNodeEditor",
		"ImGuizmo",

		"crypt32.lib",

        "%{Library.Vulkan}",
		"%{Library.fbxsdk}",
		"%{Library.libxml2}",
		"%{Library.zlib}",

		"%{Library.P4_client}",
		"%{Library.P4_api}",
		"%{Library.P4_script}",
		"%{Library.P4_script_c}",
		"%{Library.P4_script_curl}",
		"%{Library.P4_script_sqlite}",
		"%{Library.P4_rpc}",
		"%{Library.P4_supp}",

		"%{Library.OpenSSL_Crypto}",
		"%{Library.OpenSSL_SSL}"
    }

	filter "system:windows"
		systemversion "latest"

		filter "configurations:Debug"
			defines { "LP_DEBUG" }
			runtime "Debug"
			symbols "on"
			optimize "off"

            links
			{
				"%{Library.ShaderC_Debug}",
				"%{Library.ShaderC_Utils_Debug}",
				"%{Library.SPIRV_Cross_Debug}",
				"%{Library.SPIRV_Cross_GLSL_Debug}",
				"%{Library.SPIRV_Tools_Debug}",

				"%{Library.VulkanUtils}"
			}

		filter "configurations:Release"
			defines { "LP_RELEASE", "NDEBUG" }
			runtime "Release"
			symbols "on"
			optimize "on"

            links
			{
				"%{Library.ShaderC_Release}",
				"%{Library.ShaderC_Utils_Release}",
				"%{Library.SPIRV_Cross_Release}",
				"%{Library.SPIRV_Cross_GLSL_Release}",
			}

		filter "configurations:Dist"
			defines { "LP_DIST", "NDEBUG" }
			runtime "Release"
			symbols "off"
			optimize "on"
			kind "WindowedApp"

            links
			{
				"%{Library.ShaderC_Release}",
				"%{Library.ShaderC_Utils_Release}",
				"%{Library.SPIRV_Cross_Release}",
				"%{Library.SPIRV_Cross_GLSL_Release}",
			}