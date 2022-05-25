
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "MothVulkan"
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
		"vendor/yaml-cpp/include/**.h"
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
		"%{IncludeDir.fbxsdk}"
	}

	defines
	{
		"GLM_FORCE_DEPTH_ZERO_TO_ONE",
		"GLM_FORCE_SSE2",
		"GLM_FORCE_ALIGNED"
	}

	filter "files:vendor/**.cpp"
		flags {"NoPCH"}

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