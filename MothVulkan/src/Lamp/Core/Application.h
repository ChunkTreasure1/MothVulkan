#pragma once

#include "Lamp/Core/Base.h"

#include "Lamp/Rendering/Shader/Shader.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

struct GLFWwindow;

namespace Lamp
{
	struct ApplicationInfo
	{
		ApplicationInfo(const std::string& aTitle = "Lamp", uint32_t aWidth = 1280, uint32_t aHeight = 720, bool aUseVSync = true, bool aEnableImGui = true)
			: title(aTitle), width(aWidth), height(aHeight), useVSync(aUseVSync), enableImGui(aEnableImGui)
		{ }

		std::string title;
		uint32_t width;
		uint32_t height;
		bool useVSync;
		bool enableImGui;
	};

	struct MeshPushConstants
	{
		glm::vec4 data;
		glm::mat4 transform;
	};

	struct CameraData
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewProj;
	};

	struct ObjectData
	{
		glm::mat4 transform;
	};

	class Window;
	class AssetManager;
	
	class RenderPass;

	class Application
	{
	public:
		Application(const ApplicationInfo& info);
		virtual ~Application();

		void Run();
		void Shutdown();

		inline const Ref<Window> GetWindow() const { return m_window; }

		inline static Application& Get() { return *s_instance; }

	private:
		const std::vector<const char*> m_validationLayers = { "VK_LAYER_KHRONOS_validation" };

		bool m_isRunning = true;

		Ref<Window> m_window;
		Ref<AssetManager> m_assetManager;

		ApplicationInfo m_applicationInfo;
		inline static Application* s_instance;

		Ref<RenderPass> m_renderPass;
	};

	static Application* CreateApplication();
}