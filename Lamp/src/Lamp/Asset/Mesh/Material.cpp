#include "lppch.h"
#include "Material.h"

#include "Lamp/Core/Application.h"
#include "Lamp/Core/Window.h"
#include "Lamp/Core/Graphics/Swapchain.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"

#include "Lamp/Log/Log.h"

#include "Lamp/Rendering/RenderPipeline/RenderPipeline.h"

#include "Lamp/Rendering/Buffer/UniformBuffer/UniformBufferRegistry.h"
#include "Lamp/Rendering/Buffer/UniformBuffer/UniformBufferSet.h"

#include "Lamp/Rendering/Buffer/ShaderStorageBuffer/ShaderStorageBufferRegistry.h"
#include "Lamp/Rendering/Buffer/ShaderStorageBuffer/ShaderStorageBufferSet.h"

#include "Lamp/Rendering/Renderer.h"
#include "Lamp/Rendering/Framebuffer.h"

#include "Lamp/Rendering/Texture/Image2D.h"
#include "Lamp/Rendering/Texture/Texture2D.h"

namespace Lamp
{
	Material::Material(const std::string& name, uint32_t index, Ref<RenderPipeline> renderPipeline)
		: m_name(name), m_index(index), m_renderPipeline(renderPipeline)
	{
		m_renderPipeline->AddReference(this);
		SetupMaterialFromPipeline();

		CreateDescriptorPool();
		AllocateAndSetupDescriptorSets();
	}

	Material::~Material()
	{
		vkDestroyDescriptorPool(GraphicsContext::GetDevice()->GetHandle(), m_descriptorPool, nullptr);
		m_descriptorPool = nullptr;

		if (m_renderPipeline)
		{
			m_renderPipeline->RemoveReference(this);
		}
	}

	void Material::Bind(VkCommandBuffer commandBuffer, uint32_t frameIndex) const
	{
		auto device = GraphicsContext::GetDevice();
		vkUpdateDescriptorSets(device->GetHandle(), (uint32_t)m_writeDescriptors[frameIndex].size(), m_writeDescriptors[frameIndex].data(), 0, nullptr);
		
		m_renderPipeline->Bind(commandBuffer);
		
		for (uint32_t i = 0; i < (uint32_t)m_frameDescriptorSets[frameIndex].size(); i++)
		{
			m_renderPipeline->BindDescriptorSet(commandBuffer, m_frameDescriptorSets[frameIndex][i], m_descriptorSetBindings[frameIndex][i]);
		}
	}

	void Material::SetTexture(uint32_t binding, Ref<Texture2D> texture)
	{
		m_textures[binding] = texture;
		UpdateTextureWriteDescriptor(binding);
	}

	void Material::Invalidate()
	{
		if (m_descriptorPool)
		{
			vkResetDescriptorPool(GraphicsContext::GetDevice()->GetHandle(), m_descriptorPool, 0);
		}

		m_descriptorSetBindings.clear();
		m_frameDescriptorSets.clear();
		m_writeDescriptors.clear();

		m_shaderResources.clear();
		SetupMaterialFromPipeline();
		AllocateAndSetupDescriptorSets();
	}

	Ref<Material> Material::Create(const std::string& name, uint32_t index, Ref<RenderPipeline> renderPipeline)
	{
		return CreateRef<Material>(name, index, renderPipeline);
	}

	void Material::CreateDescriptorPool()
	{
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = 0;
		poolInfo.maxSets = 100; // TODO: Make this dynamic
		poolInfo.poolSizeCount = (uint32_t)m_shaderResources[0].poolSizes.size();
		poolInfo.pPoolSizes = m_shaderResources[0].poolSizes.data();

		LP_VK_CHECK(vkCreateDescriptorPool(GraphicsContext::GetDevice()->GetHandle(), &poolInfo, nullptr, &m_descriptorPool));
	}

	void Material::AllocateAndSetupDescriptorSets()
	{
		const uint32_t framesInFlight = Application::Get().GetWindow()->GetSwapchain().GetFramesInFlight();
		auto device = GraphicsContext::GetDevice();

		m_frameDescriptorSets.resize(framesInFlight);
		m_descriptorSetBindings.resize(framesInFlight);
		m_writeDescriptors.resize(framesInFlight);

		for (uint32_t i = 0; i < (uint32_t)m_frameDescriptorSets.size(); i++)
		{
			auto& sets = m_frameDescriptorSets[i];
			auto& shaderResources = m_shaderResources[i];

			VkDescriptorSetAllocateInfo allocInfo = shaderResources.setAllocInfo;
			allocInfo.descriptorPool = m_descriptorPool;
			sets.resize(allocInfo.descriptorSetCount);

			vkAllocateDescriptorSets(device->GetHandle(), &allocInfo, sets.data());

			uint32_t index = 0;
			for (auto& [set, bindings] : shaderResources.writeDescriptors)
			{
				for (auto& [binding, write] : bindings)
				{
					auto& writeDescriptor = m_writeDescriptors[i].emplace_back(write);

					if (shaderResources.uniformBuffersInfos[set].find(binding) != shaderResources.uniformBuffersInfos[set].end())
					{
						writeDescriptor.pBufferInfo = &shaderResources.uniformBuffersInfos[set][binding];
					}
					else if (shaderResources.storageBuffersInfos[set].find(binding) != shaderResources.storageBuffersInfos[set].end())
					{
						writeDescriptor.pBufferInfo = &shaderResources.storageBuffersInfos[set][binding];
					}
					else if (shaderResources.imageInfos[set].find(binding) != shaderResources.imageInfos[set].end())
					{
						writeDescriptor.pImageInfo = &shaderResources.imageInfos[set][binding];
					}

					writeDescriptor.dstSet = m_frameDescriptorSets[i][index];
					m_descriptorSetBindings[i].emplace_back(set);
				}

				index++;
			}
		}
	}

	void Material::SetupMaterialFromPipeline()
	{
		const uint32_t framesInFlight = Application::Get().GetWindow()->GetSwapchain().GetFramesInFlight();

		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			m_shaderResources.emplace_back(m_renderPipeline->GetSpecification().shader->GetResources());
		}

		for (uint32_t i = 0; i < (uint32_t)m_shaderResources.size(); i++)
		{
			for (auto& [set, bindings] : m_shaderResources[i].uniformBuffersInfos)
			{
				for (auto& [binding, info] : bindings)
				{
					Ref<UniformBuffer> ubo = UniformBufferRegistry::Get(set, binding)->Get(i);

					info.buffer = ubo->GetHandle();
					info.range = ubo->GetSize();
				}
			}

			for (auto& [set, bindings] : m_shaderResources[i].storageBuffersInfos)
			{
				for (auto& [binding, info] : bindings)
				{
					Ref<ShaderStorageBuffer> ssb = ShaderStorageBufferRegistry::Get(set, binding)->Get(i);

					info.buffer = ssb->GetHandle();
					info.range = ssb->GetSize();
				}
			}

			// TODO: add support for cubic textures
			for (auto& input : m_renderPipeline->GetSpecification().framebufferInputs)
			{
				auto& imageInfo = m_shaderResources[i].imageInfos[input.set][input.binding];
				imageInfo.imageView = m_renderPipeline->GetSpecification().framebuffer->GetColorAttachment(input.attachmentIndex)->GetView();
				imageInfo.sampler = m_renderPipeline->GetSpecification().framebuffer->GetColorAttachment(input.attachmentIndex)->GetSampler();
			}

			if (m_shaderResources[i].imageInfos.find((uint32_t)DescriptorSetType::PerMaterial) != m_shaderResources[i].imageInfos.end())
			{
				for (auto& [binding, imageInfo] : m_shaderResources[i].imageInfos[(uint32_t)DescriptorSetType::PerMaterial])
				{
					auto defaultTexture = Renderer::GetDefaultData().whiteTexture;
					imageInfo.imageView = defaultTexture->GetImage()->GetView();
					imageInfo.sampler = defaultTexture->GetImage()->GetSampler();
				}
			}
		}
	}

	void Material::UpdateTextureWriteDescriptor(uint32_t binding)
	{
		for (size_t i = 0; i < m_shaderResources.size(); i++)
		{
			m_shaderResources[i].imageInfos[(uint32_t)DescriptorSetType::PerMaterial][binding].imageView = m_textures[binding]->GetImage()->GetView();
			m_shaderResources[i].imageInfos[(uint32_t)DescriptorSetType::PerMaterial][binding].sampler = m_textures[binding]->GetImage()->GetSampler();
		}

		Invalidate();
	}
}
