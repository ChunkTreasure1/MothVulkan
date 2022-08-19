#include "lppch.h"
#include "Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace Lamp
{
	void Log::Initialize()
	{
		m_callbackSink = std::make_shared<CallbackSink<std::mutex>>();

		spdlog::set_pattern("%^[%T] %n: %v%$");

		s_clientLogger = spdlog::stdout_color_mt("APP");
		s_clientLogger->sinks().emplace_back(m_callbackSink);
		s_clientLogger->set_level(spdlog::level::trace);

		s_coreLogger = spdlog::stdout_color_mt("LAMP");
		s_coreLogger->sinks().emplace_back(m_callbackSink);
		s_coreLogger->set_level(spdlog::level::trace);
	}

	void Log::Shutdown()
	{
		m_callbackSink->ClearCallbacks();
	}

	void Log::SetLogLevel(spdlog::level::level_enum level)
	{
		s_coreLogger->set_level(level);
		s_clientLogger->set_level(level);
	}

	void Log::AddCallback(std::function<void(const LogCallbackData&)> callback)
	{
		m_callbackSink->AddCallback(callback);
	}
}