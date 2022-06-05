
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Lamp"
	location "."
	kind "StaticLib"
	language "C++"
	cppdialect "C++latest"

	targetdir ("../bin/" .. outputdir .."/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .."/%{prj.name}")

	pchheader "lppch.h"
	pchsource "src/lppch.cpp"

	disablewarnings
	{
		"4005"
	}

	linkoptions 
	{
		"/ignore:4006",
		"/ignore:4099"
	}

	files
	{
		"src/**.h",
		"src/**.cpp",
		"src/**.hpp",

		"vendor/glm/glm/**.hpp",
		"vendor/glm/glm/**.inl",

		"vendor/vma/vma/VulkanMemoryAllocator.h",
		"vendor/vma/vma/VulkanMemoryAllocator.cpp",

		"vendor/yaml-cpp/src/**.cpp",
		"vendor/yaml-cpp/src/**.h",
		"vendor/yaml-cpp/include/**.h",

		"vendor/stb/**.cpp",
		"vendor/stb/**.h",

		"%{IncludeDir.shaderc_glslc}/**.cc",
		"%{IncludeDir.shaderc_glslc}/**.h",

		"%{IncludeDir.shaderc_utils}/**.cc",
		"%{IncludeDir.shaderc_utils}/**.h"
	}

	includedirs
	{
		"src/",

		"%{IncludeDir.GLFW}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.vma}",
		"%{IncludeDir.VulkanSDK}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.yaml}",
		"%{IncludeDir.fbxsdk}",
		"%{IncludeDir.stb}",
		"%{IncludeDir.shaderc_glslc}",
		"%{IncludeDir.shaderc_utils}"
	}

	defines
	{
		"GLM_FORCE_DEPTH_ZERO_TO_ONE",
		"GLM_FORCE_SSE2",
		"GLM_FORCE_ALIGNED"
	}

	filter "files:vendor/**.cpp"
		flags {"NoPCH"}
		disablewarnings { "26451", "6387", "26812", "26439", "26800", "26495", "4717", "5232", "4067" }

	filter "files:vendor/**.h"
		disablewarnings { "26451", "6387", "26812", "26439", "26800", "26495", "4717", "5232", "4067" }

	filter "system:windows"
		systemversion "latest"

		filter "configurations:Debug"
			defines 
			{ 
				"LP_DEBUG", 
				"LP_ENABLE_ASSERTS",
				"LP_ENABLE_VALIDATION"
			}
			runtime "Debug"
			symbols "on"

		filter "configurations:Release"
			defines 
			{ 
				"LP_RELEASE", 
				"LP_ENABLE_ASSERTS",
				"LP_ENABLE_VALIDATION" 
			}
			runtime "Release"
			optimize "on"

		filter "configurations:Dist"
			defines { "LP_DIST", "NDEBUG" }
			runtime "Release"
			optimize "on"