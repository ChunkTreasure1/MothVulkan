
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

		"vendor/tiny_gltf/**.h",

		"%{IncludeDir.shaderc_glslc}/**.cc",
		"%{IncludeDir.shaderc_glslc}/**.h",

		"%{IncludeDir.shaderc_utils}/**.cc",
		"%{IncludeDir.shaderc_utils}/**.h",
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
		"%{IncludeDir.shaderc_utils}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.Wire}",
		"%{IncludeDir.Optick}",
		"%{IncludeDir.TinyGLTF}",
		"%{IncludeDir.tinyddsloader}",
		"%{IncludeDir.imgui_notify}"
	}

	defines
	{
		"GLM_FORCE_DEPTH_ZERO_TO_ONE",
		"GLM_FORCE_LEFT_HANDED",
		"GLM_FORCE_SSE2",
		"NOMINMAX"
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
				"LP_ENABLE_VALIDATION",
				"LP_ENABLE_PROFILING"
			}
			runtime "Debug"
			optimize "off"
			symbols "on"

		filter "configurations:Release"
			defines 
			{ 
				"LP_RELEASE", 
				"LP_ENABLE_ASSERTS",
				"LP_ENABLE_VALIDATION",
				"LP_ENABLE_PROFILING",
				"NDEBUG"
			}
			runtime "Release"
			optimize "on"
			symbols "on"

		filter "configurations:Dist"
			defines { "LP_DIST", "NDEBUG" }
			runtime "Release"
			optimize "on"
			symbols "off"

project "Shaders"
	location "."
	language "C++"
	kind "None"
	cppdialect "C++latest"

	targetdir ("../bin/" .. outputdir .."/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .."/%{prj.name}")

	files
	{
		"../Resources/Engine/Shaders/**.glsl",
		"../Resources/Engine/Shaders/**.glslh",

		"../Resources/Engine/Shaders/**.hlsl",
		"../Resources/Engine/Shaders/**.hlslh",
	
		"../Resources/Engine/Shaders/**.h"
	}

	filter "system:windows"
	systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		optimize "off"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"
		symbols "on"

	filter "configurations:Dist"
		runtime "Release"
		optimize "on"
		symbols "off"