#pragma once

#include "Lamp/Core/Base.h"
#include "Lamp/Core/Layer/LayerStack.h"
#include "Lamp/Event/ApplicationEvent.h"

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

	class Window;
	class AssetManager;
	class ImGuiImplementation;

	class Application
	{
	public:
		Application(const ApplicationInfo& info);
		virtual ~Application();

		void Run();
		void OnEvent(Event& event);

		void PushLayer(Layer* layer);
		
		inline const Ref<Window> GetWindow() const { return m_window; }
		inline static Application& Get() { return *s_instance; }

	private:
		bool OnWindowCloseEvent(WindowCloseEvent& e);
		bool OnWindowResizeEvent(WindowResizeEvent& e);

		const std::vector<const char*> m_validationLayers = { "VK_LAYER_KHRONOS_validation" };
		bool m_isRunning = true;

		float m_lastFrameTime = 0.f;
		float m_currentFrameTime = 0.f;

		Ref<Window> m_window;
		Ref<AssetManager> m_assetManager;
		Scope<ImGuiImplementation> m_imguiImplementation;
		
		LayerStack m_layerStack;

		ApplicationInfo m_applicationInfo;
		inline static Application* s_instance;
	};

	static Application* CreateApplication();
}