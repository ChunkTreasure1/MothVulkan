VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["GLFW"] = "%{wks.location}/Lamp/vendor/GLFW/include"
IncludeDir["ImGui"] = "%{wks.location}/Lamp/vendor/ImGui"
IncludeDir["Wire"] = "%{wks.location}/Lamp/vendor/Wire/Wire/src"
IncludeDir["spdlog"] = "%{wks.location}/Lamp/vendor/spdlog/include"
IncludeDir["vma"] = "%{wks.location}/Lamp/vendor/vma"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["glm"] = "%{wks.location}/Lamp/vendor/glm"
IncludeDir["yaml"] = "%{wks.location}/Lamp/vendor/yaml-cpp/include"
IncludeDir["fbxsdk"] = "%{wks.location}/Lamp/vendor/fbxsdk/include"
IncludeDir["stb"] = "%{wks.location}/Lamp/vendor/stb"
IncludeDir["shaderc_utils"] = "%{wks.location}/Lamp/vendor/shaderc/libshaderc_util/include"
IncludeDir["shaderc_glslc"] = "%{wks.location}/Lamp/vendor/shaderc/glslc"

LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
LibraryDir["VulkanSDK_Debug"] = "%{VULKAN_SDK}/Lib"
LibraryDir["fbxsdk"] = "%{wks.location}/Lamp/vendor/fbxsdk/lib/%{cfg.buildcfg}"

Library = {}
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK_Debug}/VkLayer_utils.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/shaderc_sharedd.lib"
Library["ShaderC_Utils_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/shaderc_utild.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/SPIRV-Toolsd.lib"

Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
Library["ShaderC_Utils_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_util.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"

Library["fbxsdk"] = "%{LibraryDir.fbxsdk}/libfbxsdk-md.lib"
Library["libxml2"] = "%{LibraryDir.fbxsdk}/libxml2-md.lib"
Library["zlib"] = "%{LibraryDir.fbxsdk}/zlib-md.lib"