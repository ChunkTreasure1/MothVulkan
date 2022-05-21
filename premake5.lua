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


group "Core"
include "MothVulkan"

group ""
include "Launcher"