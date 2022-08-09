workspace "Lamp"
	architecture "x64"
	startproject "Sandbox"

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
include "Lamp/vendor/glfw"
include "Lamp/vendor/imgui"
include "Lamp/vendor/Wire/Wire"
include "Lamp/vendor/Optick"
include "Lamp/vendor/imgui-node-editor"
include "Lamp/vendor/ImGuizmo"

group "Core"
include "Lamp"

group ""
include "Launcher"
include "Sandbox"