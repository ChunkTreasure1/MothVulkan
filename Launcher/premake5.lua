
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Launcher"
	location "."
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++latest"
	debugdir "../AssetData"

	targetdir ("../bin/" .. outputdir .."/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .."/%{prj.name}")

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
		"GLM_FORCE_SSE2"
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
		"%{IncludeDir.Optick}"
	}

    links
    {
        "Lamp",

		"GLFW",
		"ImGui",
		"Wire",
		"Optick",

        "%{Library.Vulkan}",
		"%{Library.fbxsdk}",
		"%{Library.libxml2}",
		"%{Library.zlib}"
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
			optimize "on"
			symbols "on"

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
			optimize "on"
			symbols "off"
			kind "WindowedApp"

            links
			{
				"%{Library.ShaderC_Release}",
				"%{Library.ShaderC_Utils_Release}",
				"%{Library.SPIRV_Cross_Release}",
				"%{Library.SPIRV_Cross_GLSL_Release}",
			}