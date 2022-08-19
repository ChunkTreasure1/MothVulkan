#pragma once

#include "Sandbox/Window/EditorWindow.h"

#include <Lamp/Log/CallbackSink.h>

class LogPanel : public EditorWindow
{
public:
	LogPanel();

	void UpdateMainContent() override;

private:
	uint32_t m_maxMessages = 1000;
	std::vector<LogCallbackData> m_logMessages;
};