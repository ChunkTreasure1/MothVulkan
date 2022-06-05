#pragma once

#include "Lamp/Core/Base.h"
#include "Lamp/Event/Event.h"

#include <string>

struct GLFWwindow;

namespace Lamp
{
	enum class WindowMode : uint32_t
	{
		Windowed = 0,
		Fullscreen,
		Borderless
	};

	struct WindowProperties
	{
		WindowProperties(const std::string& aTitle = "Lamp", uint32_t aWidth = 1280, uint32_t aHeight = 720, bool aVSync = true, WindowMode aWindowMode = WindowMode::Windowed)
			: title(aTitle), width(aWidth), height(aHeight), vsync(aVSync), windowMode(aWindowMode)
		{}

		std::string title;
		uint32_t width;
		uint32_t height;
		bool vsync;
		WindowMode windowMode;
	};

	class GraphicsContext;
	class Swapchain;

	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		Window(const WindowProperties& properties);
		~Window();
		
		void Shutdown();

		void Invalidate();
		void Release();

		void BeginFrame();
		void Present();

		void SetEventCallback(const EventCallbackFn& callback);
		void SetWindowMode(WindowMode windowMode);
		void Resize(uint32_t width, uint32_t height);

		void Maximize();
		void ShowCursor(bool show);

		inline const uint32_t GetWidth() const { return m_data.width; }
		inline const uint32_t GetHeight() const { return m_data.height; }
		inline const bool IsVSync() const { return m_data.vsync; }
		inline const WindowMode GetWindowMode() const { return m_data.windowMode; }
		inline GLFWwindow* GetNativeWindow() const { return m_window; }

		inline const GraphicsContext& GetGraphicsContext() const { return *m_graphicsContext; }
		inline const Swapchain& GetSwapchain() const { return *m_swapchain; }

		static Ref<Window> Create(const WindowProperties& properties = WindowProperties());

	private:
		GLFWwindow* m_window = nullptr;

		struct WindowData
		{
			std::string title;
			uint32_t width;
			uint32_t height;
			bool vsync;
			WindowMode windowMode;

			EventCallbackFn eventCallback;
			
		} m_data;

		bool m_hasBeenInitialized = false;
		
		Ref<GraphicsContext> m_graphicsContext;
		Ref<Swapchain> m_swapchain;
	};
}