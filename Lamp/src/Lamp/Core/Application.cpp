#include "lppch.h"
#include "Application.h"

#include "Lamp/Log/Log.h"
#include "Lamp/Core/Base.h"
#include "Lamp/Core/Window.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"
#include "Lamp/Core/Graphics/Swapchain.h"
#include "Lamp/Core/Layer/Layer.h"

#include "Lamp/Asset/AssetManager.h"
#include "Lamp/Asset/MaterialRegistry.h"

#include "Lamp/ImGui/ImGuiImplementation.h"

#include "Lamp/Rendering/Buffer/UniformBuffer/UniformBufferRegistry.h"
#include "Lamp/Rendering/Buffer/ShaderStorageBuffer/ShaderStorageBufferRegistry.h"

#include "Lamp/Rendering/Shader/ShaderRegistry.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipelineRegistry.h"
#include "Lamp/Rendering/RenderPass/RenderPassRegistry.h"
#include "Lamp/Rendering/Renderer.h"

#include <GLFW/glfw3.h>

namespace Lamp
{
	Application::Application(const ApplicationInfo& info)
	{
		LP_CORE_ASSERT(!s_instance, "Application already exists!");
		s_instance = this;

		m_applicationInfo = info;

		Log::Initialize();

		WindowProperties windowProperties{};
		windowProperties.width = info.width;
		windowProperties.height = info.height;
		windowProperties.vsync = info.useVSync;
		windowProperties.title = info.title;

		m_window = Window::Create(windowProperties);
		m_window->SetEventCallback(LP_BIND_EVENT_FN(Application::OnEvent));

		m_assetManager = CreateRef<AssetManager>();

		UniformBufferRegistry::Initialize();
		ShaderStorageBufferRegistry::Initialize();

		Renderer::InitializeBuffers();

		ShaderRegistry::Initialize();
		RenderPassRegistry::Initialize();
		RenderPipelineRegistry::Initialize();
		RenderPassRegistry::SetupOverrides();

		Renderer::Initialize();
		MaterialRegistry::Initialize();

		m_imguiImplementation = ImGuiImplementation::Create();
	}

	Application::~Application()
	{
		Log::Shutdown();
		vkDeviceWaitIdle(GraphicsContext::GetDevice()->GetHandle());

		m_layerStack.Clear();
		m_imguiImplementation = nullptr;

		MaterialRegistry::Shutdown();
		RenderPipelineRegistry::Shutdown();
		RenderPassRegistry::Shutdown();
		ShaderRegistry::Shutdown();
		ShaderStorageBufferRegistry::Shutdowm();
		UniformBufferRegistry::Shutdowm();

		m_assetManager = nullptr;
		Renderer::Shutdowm();
		m_window = nullptr;
		s_instance = nullptr;
	}

	void Application::Run()
	{
		LP_PROFILE_THREAD("Main");

		auto device = GraphicsContext::GetDevice()->GetHandle();

		while (m_isRunning)
		{
			LP_PROFILE_FRAME("Frame");

			m_window->BeginFrame();

			float time = (float)glfwGetTime();
			m_currentFrameTime = time - m_lastFrameTime;
			m_lastFrameTime = time;

			{
				LP_PROFILE_SCOPE("Application::Update");

				AppUpdateEvent updateEvent(m_currentFrameTime);
				OnEvent(updateEvent);
			}

			{
				LP_PROFILE_SCOPE("Application::Render");

				AppRenderEvent renderEvent{};
				OnEvent(renderEvent);
			}

			{
				LP_PROFILE_SCOPE("Application::ImGui")
				m_imguiImplementation->Begin();

				AppImGuiUpdateEvent imguiEvent{};
				OnEvent(imguiEvent);

				m_imguiImplementation->End();
			}

			m_window->Present();
		}
	}

	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowCloseEvent>(LP_BIND_EVENT_FN(Application::OnWindowCloseEvent));
		dispatcher.Dispatch<WindowResizeEvent>(LP_BIND_EVENT_FN(Application::OnWindowResizeEvent));

		//Handle rest of events
		for (auto it = m_layerStack.end(); it != m_layerStack.begin(); )
		{
			(*--it)->OnEvent(event);
			if (event.handled)
			{
				break;
			}
		}
	}

	void Application::PushLayer(Layer* layer)
	{
		m_layerStack.PushLayer(layer);
	}

	bool Application::OnWindowCloseEvent(WindowCloseEvent& e)
	{
		m_isRunning = false;
		return true;
	}

	bool Application::OnWindowResizeEvent(WindowResizeEvent& e)
	{
		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			return false;
		}

		m_window->Resize(e.GetWidth(), e.GetHeight());
		return false;
	}
}
