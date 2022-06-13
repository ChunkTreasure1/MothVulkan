#pragma once

#include "Lamp/Core/Base.h"

#include <imgui.h>
#include <imgui_stdlib.h>

namespace Lamp
{
	class Texture2D;
	class Image2D;
}

namespace UI
{
	static uint32_t s_contextId = 0;
	static uint32_t s_stackId = 0;

	class ScopedColor
	{
	public:
		ScopedColor(ImGuiCol_ color, const glm::vec4& newColor)
			: m_Color(color)
		{
			auto& colors = ImGui::GetStyle().Colors;
			m_OldColor = colors[color];
			colors[color] = ImVec4{ newColor.x, newColor.y, newColor.z, newColor.w };
		}

		~ScopedColor()
		{
			auto& colors = ImGui::GetStyle().Colors;
			colors[m_Color] = m_OldColor;
		}

	private:
		ImVec4 m_OldColor;
		ImGuiCol_ m_Color;
	};

	class ScopedStyleFloat
	{
	public:
		ScopedStyleFloat(ImGuiStyleVar_ var, float value)
		{
			ImGui::PushStyleVar(var, value);
		}

		~ScopedStyleFloat()
		{
			ImGui::PopStyleVar();
		}
	};

	class ScopedStyleFloat2
	{
	public:
		ScopedStyleFloat2(ImGuiStyleVar_ var, const glm::vec2& value)
		{
			ImGui::PushStyleVar(var, { value.x, value.y });
		}

		~ScopedStyleFloat2()
		{
			ImGui::PopStyleVar();
		}
	};

	ImTextureID GetTextureID(Ref<Lamp::Texture2D> texture);
	ImTextureID GetTextureID(Ref<Lamp::Image2D> texture);
	ImTextureID GetTextureID(Lamp::Texture2D* texture);

	static void ShiftCursor(float x, float y)
	{
		ImVec2 pos = { ImGui::GetCursorPosX() + x, ImGui::GetCursorPosY() + y };
		ImGui::SetCursorPos(pos);
	}

	static bool InputText(const std::string& name, std::string& text, ImGuiInputTextFlags_ flags = ImGuiInputTextFlags_None)
	{
		ImGui::TextUnformatted(name.c_str());
		ImGui::SameLine();

		std::string id = "##" + std::to_string(s_stackId++);
		return ImGui::InputTextString(id.c_str(), &text, flags);
	}

	static void PushId()
	{
		int id = s_contextId++;
		ImGui::PushID(id);
		s_stackId = 0;
	}

	static void PopId()
	{
		ImGui::PopID();
		s_contextId--;
	}
}
