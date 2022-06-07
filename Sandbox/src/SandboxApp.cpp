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
	}
	
	void OnEvent(Lamp::Event& event) override
	{
	}

private:
	Sandbox* m_sandbox = nullptr;
};

Lamp::Application* Lamp::CreateApplication()
{
	Lamp::ApplicationInfo info{};

	return new SandboxApp(info);
}