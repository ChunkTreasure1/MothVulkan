#include "lppch.h"
#include "MaterialInstance.h"

#include "Lamp/Asset/Mesh/Material.h"
#include "Lamp/Log/Log.h"

#include "Lamp/Core/Application.h"
#include "Lamp/Core/Window.h"
#include "Lamp/Core/Graphics/Swapchain.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"

#include "Lamp/Rendering/RenderPipeline/RenderPipeline.h"

namespace Lamp
{
	MaterialInstance::MaterialInstance(Ref<Material> sharedMaterial)
		: m_sharedMaterial(sharedMaterial)
	{
		m_sharedMaterial->AddReference(this);

		CreateDescriptorPool();
		AllocateAndSetupDescriptorSets();
	}

	MaterialInstance::~MaterialInstance()
	{
		vkDestroyDescriptorPool(GraphicsContext::GetDevice()->GetHandle(), m_descriptorPool, nullptr);
		m_descriptorPool = nullptr;

		m_sharedMaterial->RemoveReference(this);
	}

	void MaterialInstance::Bind(VkCommandBuffer cmdBuffer, uint32_t frame)
	{
		auto device = GraphicsContext::GetDevice();
		vkUpdateDescriptorSets(device->GetHandle(), (uint32_t)m_writeDescriptors[frame].size(), m_writeDescriptors[frame].data(), 0, nullptr);

		m_sharedMaterial->m_renderPipeline->Bind(cmdBuffer);

		for (size_t i = 0; i < m_frameDescriptorSets[frame].size(); i++)
		{
			m_sharedMaterial->m_renderPipeline->BindDescriptorSet(cmdBuffer, m_frameDescriptorSets[frame][i], m_descriptorSetBindings[frame][i]);
		}
	}

	void MaterialInstance::ReconstructMaterial()
	{
		vkResetDescriptorPool(GraphicsContext::GetDevice()->GetHandle(), m_descriptorPool, 0);
		m_descriptorSetBindings.clear();
		m_frameDescriptorSets.clear();
		m_writeDescriptors.clear();

		AllocateAndSetupDescriptorSets();
	}

	void MaterialInstance::CreateDescriptorPool()
	{
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = 0;
		poolInfo.maxSets = 10; // TODO: Make this dynamic
		poolInfo.poolSizeCount = (uint32_t)m_sharedMaterial->m_shaderResources[0].poolSizes.size();
		poolInfo.pPoolSizes = m_sharedMaterial->m_shaderResources[0].poolSizes.data();

		LP_VK_CHECK(vkCreateDescriptorPool(GraphicsContext::GetDevice()->GetHandle(), &poolInfo, nullptr, &m_descriptorPool));
	}

	void MaterialInstance::AllocateAndSetupDescriptorSets()
	{
		const uint32_t framesInFlight = Application::Get().GetWindow()->GetSwapchain().GetFramesInFlight();
		auto device = GraphicsContext::GetDevice();

		m_frameDescriptorSets.resize(framesInFlight);
		m_descriptorSetBindings.resize(framesInFlight);
		m_writeDescriptors.resize(framesInFlight);

		for (uint32_t i = 0; i < (uint32_t)m_frameDescriptorSets.size(); i++)
		{
			auto& sets = m_frameDescriptorSets[i];
			auto& shaderResources = m_sharedMaterial->m_shaderResources[i];

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
}