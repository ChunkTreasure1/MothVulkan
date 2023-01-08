#pragma once

#include "Lamp/Core/Base.h"

#include <vulkan/vulkan.h>

namespace Lamp
{
	class Texture2D;
	class TextureTable
	{
	public:
		TextureTable(uint32_t maxTextureCount, uint32_t binding);
		~TextureTable();

		const int32_t GetBindingFromTexture(const Ref<Texture2D> texture);
		const uint32_t AddTexture(const Ref<Texture2D> texture);
		void RemoveTexture(const Ref<Texture2D> texture);

		static Ref<TextureTable> Create(uint32_t maxTextureCount, uint32_t binding);

	private:
		uint32_t m_maxTextureCount = 0;
		const uint32_t m_binding;

		std::unordered_map<Ref<Texture2D>, uint32_t> m_textureToIndexMap;

		VkDescriptorSet m_descriptorSet = nullptr;
		VkDescriptorSetLayout m_descriptorLayout = nullptr;
		VkDescriptorPool m_tablePool = nullptr;
	};
}