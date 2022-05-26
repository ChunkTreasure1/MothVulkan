#include "lppch.h"
#include "Window.h"

#include "Lamp/Log/Log.h"
#include "Lamp/Core/Application.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/Swapchain.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace Lamp
{
	static void GLFWErrorCallback(int error, const char* description)
	{
		LP_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

	Window::Window(const WindowProperties& properties)
		: m_properties(properties)
	{
		Invalidate();
	}

	Window::~Window()
	{
		Shutdown();
	}

	void Window::BeginFrame()
	{
		m_swapchain->BeginFrame();
	}

	void Window::Present()
	{
		m_swapchain->Present();
		glfwPollEvents();
	}

	void Window::Invalidate()
	{
		if (m_window)
		{
			Release();
		}

		if (!m_hasBeenInitialized)
		{
			if (!glfwInit())
			{
				LP_CORE_ERROR("Failed to initialize GLFW");
				return;
			}
		}

		glfwSetErrorCallback(GLFWErrorCallback);
		glfwWindowHint(GLFW_SAMPLES, 0);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);	
		glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
		
		m_window = glfwCreateWindow((int32_t)m_properties.width, (int32_t)m_properties.height, m_properties.title.c_str(), nullptr, nullptr);
	
		if (m_properties.windowMode != WindowMode::Windowed)
		{
			SetWindowMode(m_properties.windowMode);
		}

		if (!m_hasBeenInitialized)
		{
			m_graphicsContext = GraphicsContext::Create();
			m_swapchain = Swapchain::Create(m_window);
			m_swapchain->Resize(m_properties.width, m_properties.height, m_properties.vsync);
			m_hasBeenInitialized = true;
		}

		glfwSetWindowUserPointer(m_window, this);
		glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window)
			{
				Application::Get().Shutdown();
			});
	}

	void Window::Shutdown()
	{
		m_swapchain = nullptr;
		m_graphicsContext = nullptr;
		Release();
		glfwTerminate();
	}

	void Window::Resize(uint32_t width, uint32_t height)
	{
		m_swapchain->Resize(width, height, m_properties.vsync);
	}

	void Window::SetWindowMode(WindowMode windowMode)
	{
		m_properties.windowMode = windowMode;
		
		switch (windowMode)
		{
			case WindowMode::Fullscreen:
			{
				const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
				glfwSetWindowMonitor(m_window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
				break;
			}

			case WindowMode::Windowed:
			{
				glfwSetWindowMonitor(m_window, nullptr, 0, 0, m_properties.width, m_properties.height, 0);
				break;
			}
			
			case WindowMode::Borderless:
			{
				const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
				glfwSetWindowMonitor(m_window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
				break;
			}
		}
	}

	void Window::ShowCursor(bool show)
	{
		glfwSetInputMode(m_window, GLFW_CURSOR, show ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}

	void Window::Release()
	{
		if (m_window)
		{
			glfwDestroyWindow(m_window);
			m_window = nullptr;
		}
	}

	void Window::Maximize()
	{
		glfwMaximizeWindow(m_window);
	}

	Ref<Window> Window::Create(const WindowProperties& properties)
	{
		return CreateRef<Window>(properties);
	}
}