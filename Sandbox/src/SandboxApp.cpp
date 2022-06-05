#include <Lamp/Core/Application.h>
#include <Lamp/EntryPoint.h>

class SandboxApp : public Lamp::Application
{
public:
	SandboxApp(const Lamp::ApplicationInfo& appInfo)
		: Lamp::Application(appInfo)
	{
	}

private:
};

Lamp::Application* Lamp::CreateApplication()
{
	Lamp::ApplicationInfo info{};

	return new SandboxApp(info);
}