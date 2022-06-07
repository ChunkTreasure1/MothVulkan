#include <Lamp/Core/Application.h>
#include <Lamp/EntryPoint.h>

class LauncherApp : public Lamp::Application
{
public:
	LauncherApp(const Lamp::ApplicationInfo& appInfo)
		: Lamp::Application(appInfo)
	{ }

	void OnEvent(Lamp::Event& event) override {}
	void OnAttach() override {}
	void OnDetach() override {}

private:
};

Lamp::Application* Lamp::CreateApplication()
{
	Lamp::ApplicationInfo info{};

	return new LauncherApp(info);
}