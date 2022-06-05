#pragma once

#include "Event.h"

#include <sstream>
#include <vector>
#include <filesystem>

namespace Lamp
{
	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(uint32_t width, uint32_t height)
			: m_width(width), m_height(height)
		{
		}
		//Getting
		inline const uint32_t GetWidth() const { return m_width; }
		inline const uint32_t GetHeight() const { return m_height; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowResizeEvent: " << m_width << ", " << m_height << std::endl;
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowResize);
		EVENT_CLASS_CATEGORY(EventCategoryApplication);

	private:
		uint32_t m_width;
		uint32_t m_height;
	};

	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() = default;

		EVENT_CLASS_TYPE(WindowClose);
		EVENT_CLASS_CATEGORY(EventCategoryApplication);
	};

	class AppUpdateEvent : public Event
	{
	public:
		AppUpdateEvent(float timestep)
			: m_timestep(timestep)
		{
		}

		inline const float& GetTimestep() { return m_timestep; }

		EVENT_CLASS_TYPE(AppUpdate);
		EVENT_CLASS_CATEGORY(EventCategoryApplication);

	private:
		float m_timestep;
	};

	class AppRenderEvent : public Event
	{
	public:
		AppRenderEvent() = default;

		EVENT_CLASS_TYPE(AppRender);
		EVENT_CLASS_CATEGORY(EventCategoryApplication);
	};

	class AppLogEvent : public Event
	{
	public:
		AppLogEvent(const std::string& message, const std::string& severity)
			: m_message(message), m_severity(severity)
		{}

		inline const std::string& GetMessage() { return m_message; }
		inline const std::string& GetSeverity() { return m_severity; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "AppLogEvent: " << m_message << ", " << m_severity << std::endl;
			return ss.str();
		}

		EVENT_CLASS_TYPE(AppLog);
		EVENT_CLASS_CATEGORY(EventCategoryApplication);

	private:
		std::string m_message;
		std::string m_severity;
	};

	class WindowDragDropEvent : public Event
	{
	public:
		WindowDragDropEvent(int32_t count, const char** paths)
		{
			for (int32_t i = 0; i < count; ++i)
			{
				m_paths.push_back(paths[i]);
			}
		}

		inline const std::vector <std::filesystem::path>& GetPaths() const { return m_paths; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "EditorDragDrop Accepted!" << std::endl;
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowDragDrop);
		EVENT_CLASS_CATEGORY(EventCategoryApplication);

	private:
		std::vector<std::filesystem::path> m_paths;
	};
}