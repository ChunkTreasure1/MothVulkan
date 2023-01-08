#include "lppch.h"
#include "TextureTable.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"

#include "Lamp/Rendering/Renderer.h"
#include "Lamp/Rendering/Texture/Texture2D.h"

namespace Lamp
{
	TextureTable::TextureTable(uint32_t maxTextureCount, uint32_t binding)
		: m_maxTextureCount(maxTextureCount), m_binding(binding)
	{
		// Pool
		{
			VkDescriptorPoolSize poolSizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_maxTextureCount }
			};

			VkDescriptorPoolCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
			info.maxSets = m_maxTextureCount;
			info.poolSizeCount = 1;
			info.pPoolSizes = poolSizes;

			auto device = GraphicsContext::GetDevice();
			LP_VK_CHECK(vkCreateDescriptorPool(device->GetHandle(), &info, nullptr, &m_tablePool));
		}

		// Descriptor set layout
		{
			VkDescriptorSetLayoutBinding binding{};

			binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			binding.descriptorCount = m_maxTextureCount;
			binding.stageFlags = VK_SHADER_STAGE_ALL;
			binding.binding = m_binding;
			binding.pImmutableSamplers = nullptr;

			VkDescriptorBindingFlags flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
			VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo{};
			extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			extendedInfo.bindingCount = 1;
			extendedInfo.pBindingFlags = &flags;

			VkDescriptorSetLayoutCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			info.pNext = &extendedInfo;
			info.bindingCount = 1;
			info.pBindings = &binding;
			info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

			auto device = GraphicsContext::GetDevice();
			LP_VK_CHECK(vkCreateDescriptorSetLayout(device->GetHandle(), &info, nullptr, &m_descriptorLayout));
		}

		// Descriptor set
		{
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &m_descriptorLayout;
			allocInfo.descriptorPool = m_tablePool;

			const uint32_t maxBinding = m_maxTextureCount - 1;

			VkDescriptorSetVariableDescriptorCountAllocateInfo countInfo{};
			countInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
			countInfo.descriptorSetCount = 1;
			countInfo.pDescriptorCounts = &maxBinding;

			allocInfo.pNext = &countInfo;
			
			auto device = GraphicsContext::GetDevice();
			LP_VK_CHECK(vkAllocateDescriptorSets(device->GetHandle(), &allocInfo, &m_descriptorSet));
		}
	}

	TextureTable::~TextureTable()
	{
		if (m_tablePool)
		{
			auto device = GraphicsContext::GetDevice();

			vkDestroyDescriptorSetLayout(device->GetHandle(), m_descriptorLayout, nullptr);
			vkDestroyDescriptorPool(device->GetHandle(), m_tablePool, nullptr);
			
			m_tablePool = nullptr;
		}

		m_textureToIndexMap.clear();
	}

	const int32_t TextureTable::GetBindingFromTexture(const Ref<Texture2D> texture)
	{
		if (m_textureToIndexMap.contains(texture))
		{
			return (int32_t)m_textureToIndexMap.at(texture);
		}

		return -1;
	}

	const uint32_t TextureTable::AddTexture(const Ref<Texture2D> texture)
	{
		if (m_textureToIndexMap.contains(texture))
		{
			return m_textureToIndexMap.at(texture);
		}

		const uint32_t index = (uint32_t)m_textureToIndexMap.size();

		m_textureToIndexMap.emplace(texture, index);

		Renderer::SubmitInvalidation([texture, index, descriptorSet = m_descriptorSet, binding = m_binding]()
			{
				VkDescriptorImageInfo imageInfo{};
				imageInfo.sampler = texture->GetImage()->GetSampler();
				imageInfo.imageView = texture->GetImage()->GetView();
				imageInfo.imageLayout = texture->GetImage()->GetLayout();

				VkWriteDescriptorSet writeDescriptor{};
				writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptor.descriptorCount = 1;
				writeDescriptor.dstArrayElement = index;
				writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptor.dstSet = descriptorSet;
				writeDescriptor.dstBinding = binding;

				writeDescriptor.pImageInfo = &imageInfo;

				auto device = GraphicsContext::GetDevice();
				vkUpdateDescriptorSets(device->GetHandle(), 1, &writeDescriptor, 0, nullptr);
			});

		return index;
	}

	void TextureTable::RemoveTexture(const Ref<Texture2D> texture)
	{
		if (m_textureToIndexMap.contains(texture))
		{
			m_textureToIndexMap.erase(texture);
		}
	}

	Ref<TextureTable> TextureTable::Create(uint32_t maxTextureCount, uint32_t binding)
	{
		return CreateRef<TextureTable>(maxTextureCount, binding);
	}
}