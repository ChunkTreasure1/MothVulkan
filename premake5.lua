workspace "MothVulkan"
	architecture "x64"
	startproject "Launcher"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}
	
	flags
	{
		"MultiProcessorCompile"
	}
	
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "Dependencies.lua"

group "Dependencies"
include "MothVulkan/vendor/glfw"


group "Core"
include "MothVulkan"

group ""
include "Launcher"