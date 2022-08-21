#pragma once

#include "Lamp/Core/Base.h"

#include <vulkan/vulkan.h>

struct ImFont;
namespace Lamp
{
	class ImGuiImplementation
	{
	public:
		ImGuiImplementation();
		~ImGuiImplementation();

		void Begin();
		void End();
		
		static Scope<ImGuiImplementation> Create();

	private:
		std::filesystem::path GetOrCreateIniPath();

		ImFont* m_font;
		VkDescriptorPool m_descriptorPool;
	};
}