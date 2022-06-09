#include "lppch.h"
#include "UIUtility.h"

#include "Lamp/Rendering/Texture/Texture2D.h"
#include "Lamp/Rendering/Texture/Image2D.h"

#include <vulkan/vulkan.h>
#include <backends/imgui_impl_vulkan.h>

namespace UI
{
	ImTextureID GetTextureID(Ref<Lamp::Texture2D> texture)
	{
		ImTextureID id = ImGui_ImplVulkan_AddTexture(texture->GetImage()->GetSampler(), texture->GetImage()->GetView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		return id;
	}

	ImTextureID GetTextureID(Ref<Lamp::Image2D> texture)
	{
		ImTextureID id = ImGui_ImplVulkan_AddTexture(texture->GetSampler(), texture->GetView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		return id;
	}

	ImTextureID GetTextureID(Lamp::Texture2D* texture)
	{
		ImTextureID id = ImGui_ImplVulkan_AddTexture(texture->GetImage()->GetSampler(), texture->GetImage()->GetView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		return id;
	}
}