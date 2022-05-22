#pragma once

#include "Lamp/Core/Application.h"
#include "Lamp/Log/Log.h"

extern Lamp::Application* Lamp::CreateApplication();

namespace Lamp
{
	int Main()
	{
		Lamp::Log::Initialize();

		Application* app = Lamp::CreateApplication();
		app->Run();

		delete app;

		Lamp::Log::Shutdown();

		return 0;
	}
}

#ifdef LP_DIST

#include <Windows.h>

int APIENTRY WinMain(HINSTANCE aHInstance, HINSTANCE aPrevHInstance, PSTR aCmdLine, int aCmdShow)
{
	return Lamp::Main();
}

#else

int main()
{
	return Lamp::Main();
}

#endif
	