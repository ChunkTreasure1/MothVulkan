
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "MothVulkan"
	location "."
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"

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
	}

	includedirs
	{
		"src/",
		"vendor/",
		"%{IncludeDir.GLFW}"
	}

	filter "system:windows"
		systemversion "latest"

		filter "configurations:Debug"
			defines { "LP_DEBUG", "LP_ENABLE_ASSERTS" }
			runtime "Debug"
			symbols "on"

		filter "configurations:Release"
			defines { "LP_RELEASE", "LP_ENABLE_ASSERTS" }
			runtime "Release"
			optimize "on"

		filter "configurations:Dist"
			defines { "LP_DIST", "NDEBUG" }
			runtime "Release"
			optimize "on"