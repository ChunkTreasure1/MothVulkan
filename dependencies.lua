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
IncludeDir["Optick"] = "%{wks.location}/Lamp/vendor/Optick/src"
IncludeDir["TinyGLTF"] = "%{wks.location}/Lamp/vendor/tiny_gltf/"
IncludeDir["tinyddsloader"] = "%{wks.location}/Lamp/vendor/tinyddsloader/"
IncludeDir["imgui_notify"] = "%{wks.location}/Lamp/vendor/imgui-notify/"
IncludeDir["imgui_node_editor"] = "%{wks.location}/Lamp/vendor/imgui-node-editor/"
IncludeDir["ImGuizmo"] = "%{wks.location}/Lamp/vendor/ImGuizmo/"
IncludeDir["P4"] = "%{wks.location}/Sandbox/vendor/p4/include/"

LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
LibraryDir["VulkanSDK_Debug"] = "%{VULKAN_SDK}/Lib"
LibraryDir["fbxsdk"] = "%{wks.location}/Lamp/vendor/fbxsdk/lib/%{cfg.buildcfg}"
LibraryDir["P4"] = "%{wks.location}/Sandbox/vendor/p4/lib/%{cfg.buildcfg}"
LibraryDir["OpenSSL"] = "%{wks.location}/Sandbox/vendor/OpenSSL/lib/%{cfg.buildcfg}"

Library = {}
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK_Debug}/VkLayer_utils.lib"
Library["dxc"] = "%{LibraryDir.VulkanSDK}/dxcompiler.lib"

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

Library["P4_client"] = "%{LibraryDir.P4}/libclient.lib"
Library["P4_api"] = "%{LibraryDir.P4}/libp4api.lib"
Library["P4_script"] = "%{LibraryDir.P4}/libp4script.lib"
Library["P4_script_c"] = "%{LibraryDir.P4}/libp4script_c.lib"
Library["P4_script_curl"] = "%{LibraryDir.P4}/libp4script_curl.lib"
Library["P4_script_sqlite"] = "%{LibraryDir.P4}/libp4script_sqlite.lib"
Library["P4_rpc"] = "%{LibraryDir.P4}/librpc.lib"
Library["P4_supp"] = "%{LibraryDir.P4}/libsupp.lib"

Library["OpenSSL_Crypto"] = "%{LibraryDir.OpenSSL}/libcrypto.lib"
Library["OpenSSL_SSL"] = "%{LibraryDir.OpenSSL}/libssl.lib"


