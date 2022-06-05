#include "lppch.h"
#include "Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace Lamp
{
	void Log::Initialize()
	{
		spdlog::set_pattern("%^[%T] %n: %v%$");

		s_clientLogger = spdlog::stdout_color_mt("APP");
		s_clientLogger->set_level(spdlog::level::trace);

		s_coreLogger = spdlog::stdout_color_mt("LAMP");
		s_coreLogger->set_level(spdlog::level::trace);
	}

	void Log::Shutdown()
	{
	}

}