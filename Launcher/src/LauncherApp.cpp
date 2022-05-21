#include <Lamp/Core/Application.h>
#include <Lamp/EntryPoint.h>

class LauncherApp : public Lamp::Application
{
public:
	LauncherApp(const Lamp::ApplicationInfo& appInfo)
		: Lamp::Application(appInfo)
	{ }

private:
};

Lamp::Application* Lamp::CreateApplication()
{
	Lamp::ApplicationInfo info{};

	return new LauncherApp(info);
}