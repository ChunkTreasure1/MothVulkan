#include "sbpch.h"

#include "Sandbox/Sandbox.h"

#include <Lamp/Core/Application.h>
#include <Lamp/EntryPoint.h>


class SandboxApp : public Lamp::Application
{
public:
	SandboxApp(const Lamp::ApplicationInfo& appInfo)
		: Lamp::Application(appInfo)
	{
		PushLayer(new Sandbox());
	}
};

Lamp::Application* Lamp::CreateApplication()
{
	Lamp::ApplicationInfo info{};
	info.useVSync = false;

	return new SandboxApp(info);
}