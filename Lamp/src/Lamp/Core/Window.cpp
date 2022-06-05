#include "lppch.h"
#include "Window.h"

#include "Lamp/Log/Log.h"
#include "Lamp/Core/Application.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/Swapchain.h"

#include "Lamp/Event/ApplicationEvent.h"
#include "Lamp/Event/KeyEvent.h"
#include "Lamp/Event/MouseEvent.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace Lamp
{
	static void GLFWErrorCallback(int error, const char* description)
	{
		LP_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

	Window::Window(const WindowProperties& properties)
	{
		m_data.height = properties.height;
		m_data.width = properties.width;
		m_data.title = properties.title;
		m_data.vsync = properties.vsync;

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
		
		m_window = glfwCreateWindow((int32_t)m_data.width, (int32_t)m_data.height, m_data.title.c_str(), nullptr, nullptr);
	
		if (m_data.windowMode != WindowMode::Windowed)
		{
			SetWindowMode(m_data.windowMode);
		}

		if (!m_hasBeenInitialized)
		{
			m_graphicsContext = GraphicsContext::Create();
			m_swapchain = Swapchain::Create(m_window);
			m_swapchain->Resize(m_data.width, m_data.height, m_data.vsync);
			m_hasBeenInitialized = true;
		}

		glfwSetWindowUserPointer(m_window, &m_data);

		glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int32_t width, int32_t height) 
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				data.width = width;
				data.height = height;

				WindowResizeEvent event(width, height);
				data.eventCallback(event);
			});

		glfwSetDropCallback(m_window, [](GLFWwindow* window, int32_t count, const char** paths)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				WindowDragDropEvent event(count, paths);
				data.eventCallback(event);
			});

		glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window) 
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				WindowCloseEvent event{};
				data.eventCallback(event);
			});

		glfwSetKeyCallback(m_window, [](GLFWwindow* window, int32_t key, int32_t scanCode, int32_t action, int32_t mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
					case GLFW_PRESS:
					{
						KeyPressedEvent event(key, 0);
						data.eventCallback(event);
						break;
					}

					case GLFW_RELEASE:
					{
						KeyReleasedEvent event(key);
						data.eventCallback(event);
						break;
					}

					case GLFW_REPEAT:
					{
						KeyPressedEvent event(key, 1);
						data.eventCallback(event);
						break;
					}
				}
			});

		glfwSetCharCallback(m_window, [](GLFWwindow* window, uint32_t key)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
				KeyTypedEvent event(key);
				data.eventCallback(event);
			});

		glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
				switch (action)
				{
					case GLFW_PRESS:
					{
						MouseButtonPressedEvent event(button);
						data.eventCallback(event);
						break;
					}
					case GLFW_RELEASE:
					{
						MouseButtonReleasedEvent event(button);
						data.eventCallback(event);
						break;
					}
				}
			});

		glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

				MouseScrolledEvent event((float)xOffset, (float)yOffset);
				data.eventCallback(event);
			});

		glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xPos, double yPos)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

				MouseMovedEvent event((float)xPos, (float)yPos);
				data.eventCallback(event);
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
		m_swapchain->Resize(width, height, m_data.vsync);
	}

	void Window::SetEventCallback(const EventCallbackFn& callback)
	{
		m_data.eventCallback = callback;
	}

	void Window::SetWindowMode(WindowMode windowMode)
	{
		m_data.windowMode = windowMode;
		
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
				glfwSetWindowMonitor(m_window, nullptr, 0, 0, m_data.width, m_data.height, 0);
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