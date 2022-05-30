#pragma once

#include "Lamp/Core/Base.h"

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

	class Window;
	class VertexBuffer;
	class AssetManager;
	
	class Application
	{
	public:
		Application(const ApplicationInfo& info);
		virtual ~Application();

		void Run();
		void Shutdown();

		inline static Application& Get() { return *s_instance; }

	private:
		void CreatePipeline();
		void CreateTriangle();

		const std::vector<const char*> m_validationLayers = { "VK_LAYER_KHRONOS_validation" };

		bool m_isRunning = true;

		Ref<Window> m_window;
		Ref<AssetManager> m_assetManager;

		ApplicationInfo m_applicationInfo;
		inline static Application* s_instance;

		//////Pipeline/////
		VkPipelineLayout m_pipelineLayout;
		VkPipeline m_pipeline;

		uint32_t m_frameNumber = 0;
		///////////////////
		
		/////Meshes/////
		Ref<VertexBuffer> m_vertexBuffer;
		////////////////
	};

	static Application* CreateApplication();
}