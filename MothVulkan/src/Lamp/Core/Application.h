#pragma once

#include <string>

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

	class Application
	{
	public:
		Application(const ApplicationInfo& info);
		virtual ~Application();

		void Run();
	};

	static Application* CreateApplication();
}