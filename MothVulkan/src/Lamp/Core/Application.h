#pragma once

#include "Lamp/Core/Base.h"

#include <vulkan/vulkan.h>

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

	class Window;
	class Application
	{
	public:
		Application(const ApplicationInfo& info);
		virtual ~Application();

		void Run();

	private:
		void CreatePipeline();

		const std::vector<const char*> m_validationLayers = { "VK_LAYER_KHRONOS_validation" };

		bool m_isRunning = true;

		Ref<Window> m_window;

		ApplicationInfo m_applicationInfo;

		//////Pipeline/////
		VkPipelineLayout m_pipelineLayout;
		VkPipeline m_pipeline;
		///////////////////
	};

	static Application* CreateApplication();
}