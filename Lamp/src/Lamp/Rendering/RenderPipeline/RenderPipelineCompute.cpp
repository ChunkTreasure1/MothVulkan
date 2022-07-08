#include "lppch.h"
#include "RenderPipelineCompute.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"
#include "Lamp/Log/Log.h"

#include "Lamp/Rendering/Buffer/UniformBuffer/UniformBuffer.h"
#include "Lamp/Rendering/Buffer/UniformBuffer/UniformBufferSet.h"

#include "Lamp/Rendering/Buffer/ShaderStorageBuffer/ShaderStorageBuffer.h"
#include "Lamp/Rendering/Buffer/ShaderStorageBuffer/ShaderStorageBufferSet.h"

#include "Lamp/Rendering/Texture/Image2D.h"
#include "Lamp/Rendering/Texture/Texture2D.h"
#include "Lamp/Rendering/Renderer.h"

#include "Lamp/Utility/ImageUtility.h"

namespace Lamp
{
	RenderPipelineCompute::RenderPipelineCompute(Ref<Shader> computeShader, uint32_t count)
		: m_shader(computeShader), m_count(count)
	{
		CreatePipeline();
		AllocateAndSetupDescriptorsAndBarriers();
	}

	RenderPipelineCompute::~RenderPipelineCompute()
	{
		Renderer::SubmitResourceFree([pipelineLayout = m_pipelineLayout, pipelineCache = m_pipelineCache, pipeline = m_pipeline, descriptorPool = m_descriptorPool]()
			{
				auto device = GraphicsContext::GetDevice();

				vkDestroyDescriptorPool(device->GetHandle(), descriptorPool, nullptr);
				vkDestroyPipelineCache(device->GetHandle(), pipelineCache, nullptr);
				vkDestroyPipeline(device->GetHandle(), pipeline, nullptr);
				vkDestroyPipelineLayout(device->GetHandle(), pipelineLayout, nullptr);
			});
	}

	void RenderPipelineCompute::Bind(VkCommandBuffer commandBuffer, uint32_t frameIndex)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
	}

	void RenderPipelineCompute::InsertBarrier(VkCommandBuffer commandBuffer, uint32_t frameIndex, VkPipelineStageFlags pipelineStage)
	{
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, pipelineStage, 0, 0, nullptr,
			(uint32_t)m_bufferBarriers[frameIndex].size(), m_bufferBarriers[frameIndex].data(),
			(uint32_t)m_imageBarriers[frameIndex].size(), m_imageBarriers[frameIndex].data());
	}

	void RenderPipelineCompute::Dispatch(VkCommandBuffer commandBuffer, uint32_t index, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		WriteAndBindDescriptors(commandBuffer, index);
		vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
	}

	void RenderPipelineCompute::SetUniformBuffer(Ref<UniformBufferSet> uniformBuffer, uint32_t set, uint32_t binding)
	{
		if (m_shaderResources[0].uniformBuffersInfos.find(set) == m_shaderResources[0].uniformBuffersInfos.end() ||
			m_shaderResources[0].uniformBuffersInfos.at(set).find(binding) == m_shaderResources[0].uniformBuffersInfos.at(set).end())
		{
			LP_CORE_ERROR("[RenderPipelineCompute] Unable to set buffer at set {0} and binding {1}", set, binding);
			return;
		}

		for (uint32_t i = 0; i < (uint32_t)m_shaderResources.size(); i++)
		{
			auto& info = m_shaderResources[i].uniformBuffersInfos[set][binding];

			Ref<UniformBuffer> ubo = uniformBuffer->Get(i);
			info.buffer = ubo->GetHandle();
			info.range = ubo->GetSize();
		}
	}

	void RenderPipelineCompute::SetStorageBuffer(Ref<ShaderStorageBufferSet> storageBuffer, uint32_t set, uint32_t binding, VkAccessFlags accessFlags)
	{
		if (m_shaderResources[0].storageBuffersInfos.find(set) == m_shaderResources[0].storageBuffersInfos.end() ||
			m_shaderResources[0].storageBuffersInfos.at(set).find(binding) == m_shaderResources[0].storageBuffersInfos.at(set).end())
		{
			LP_CORE_ERROR("[RenderPipelineCompute] Unable to set buffer at set {0} and binding {1}", set, binding);
			return;
		}

		for (uint32_t i = 0; i < (uint32_t)m_shaderResources.size(); i++)
		{
			auto& info = m_shaderResources[i].storageBuffersInfos[set][binding];

			Ref<ShaderStorageBuffer> ssbo = storageBuffer->Get(i);
			info.info.buffer = ssbo->GetHandle();
			info.info.range = ssbo->GetSize();

			auto it = std::find_if(m_bufferBarriers[i].begin(), m_bufferBarriers[i].end(), [this, set, binding, i, ssbo](const VkBufferMemoryBarrier& barrier)
				{
					Ref<ShaderStorageBuffer> oldSSBO;
					if (m_storageBufferSets[set].find(binding) != m_storageBufferSets[set].end())
					{
						oldSSBO = m_storageBufferSets[set][binding]->Get(i);
					}

					return (oldSSBO && barrier.buffer == oldSSBO->GetHandle()) || barrier.buffer == VK_NULL_HANDLE || barrier.buffer == ssbo->GetHandle();
				});

			if (it != m_bufferBarriers[i].end() && info.writeable)
			{
				it->buffer = ssbo->GetHandle();
				it->size = ssbo->GetSize();
				it->dstAccessMask = accessFlags;
			}
			else
			{
				LP_CORE_ASSERT(false, "No valid buffer barrier found!");
			}
		}

		m_storageBufferSets[set][binding] = storageBuffer;
	}

	void RenderPipelineCompute::SetPushConstant(VkCommandBuffer cmdBuffer, uint32_t size, const void* data, uint32_t offset) const
	{
		vkCmdPushConstants(cmdBuffer, m_pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, offset, size, data);
	}

	void RenderPipelineCompute::SetTexture(Ref<Texture2D> texture, uint32_t dstSet, uint32_t dstBinding, uint32_t srcMip)
	{
		UpdateImage(texture->GetImage(), dstSet, dstBinding, srcMip, ImageUsage::Texture);
	}

	void RenderPipelineCompute::SetImage(Ref<Image2D> image, uint32_t dstSet, uint32_t dstBinding, uint32_t srcMip, VkAccessFlags dstAccessFlags, VkImageLayout targetLayout)
	{
		UpdateImage(image, dstSet, dstBinding, srcMip, ImageUsage::Storage, dstAccessFlags, targetLayout);
	}

	void RenderPipelineCompute::SetImage(Ref<Image2D> image, uint32_t dstSet, uint32_t dstBinding, uint32_t srcMip)
	{
		UpdateImage(image, dstSet, dstBinding, srcMip, ImageUsage::Texture);
	}

	void RenderPipelineCompute::UpdateImage(Ref<Image2D> image, uint32_t dstSet, uint32_t dstBinding, uint32_t srcMip, ImageUsage usage, VkAccessFlags dstAccessFlags, VkImageLayout targetLayout)
	{
		if (usage == ImageUsage::Texture)
		{
			if (m_shaderResources[0].imageInfos.find(dstSet) == m_shaderResources[0].imageInfos.end() ||
				m_shaderResources[0].imageInfos.at(dstSet).find(dstBinding) == m_shaderResources[0].imageInfos.at(dstSet).end())
			{
				LP_CORE_ERROR("[RenderPipelineCompute] Unable to set texture at set {0} and binding {1}", dstSet, dstBinding);
				return;
			}

			for (uint32_t i = 0; i < (uint32_t)m_shaderResources.size(); i++)
			{
				auto& info = m_shaderResources[i].imageInfos[dstSet][dstBinding].info;
				info.imageView = image->GetView(srcMip);
				info.sampler = image->GetSampler();
			}
		}
		else if (usage == ImageUsage::Storage)
		{
			if (m_shaderResources[0].storageImagesInfos.find(dstSet) == m_shaderResources[0].storageImagesInfos.end() ||
				m_shaderResources[0].storageImagesInfos.at(dstSet).find(dstBinding) == m_shaderResources[0].storageImagesInfos.at(dstSet).end())
			{
				LP_CORE_ERROR("[RenderPipelineCompute] Unable to set image at set {0} and binding {1}", dstSet, dstBinding);
				return;
			}

			for (uint32_t i = 0; i < (uint32_t)m_shaderResources.size(); i++)
			{
				auto& info = m_shaderResources[i].storageImagesInfos[dstSet][dstBinding];
				info.info.imageView = image->GetView(srcMip);
				info.info.sampler = image->GetSampler();

				auto it = std::find_if(m_imageBarriers[i].begin(), m_imageBarriers[i].end(), [this, dstSet, dstBinding, i, image](const VkImageMemoryBarrier& barrier)
					{
						Ref<Image2D> oldImage;
						if (m_images[dstSet].find(dstBinding) != m_images[dstSet].end())
						{
							oldImage = m_images[dstSet][dstBinding];
						}

						return (oldImage && barrier.image == oldImage->GetHandle()) || barrier.image == VK_NULL_HANDLE || barrier.image == image->GetHandle();
					});

				if (it != m_imageBarriers[i].end() && info.writeable)
				{
					it->image = image->GetHandle();
					it->oldLayout = image->GetLayout();
					it->newLayout = targetLayout == VK_IMAGE_LAYOUT_UNDEFINED ? image->GetLayout() : targetLayout;
					it->dstAccessMask = dstAccessFlags;
					it->subresourceRange.aspectMask = Utility::IsDepthFormat(image->GetFormat()) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

					image->m_imageLayout = it->newLayout;
				}
				else
				{
					LP_CORE_ASSERT(false, "No valid image barrier found!");
				}
			}

			m_images[dstSet][dstBinding] = image;
		}
	}

	Ref<RenderPipelineCompute> RenderPipelineCompute::Create(Ref<Shader> computeShader, uint32_t count)
	{
		return CreateRef<RenderPipelineCompute>(computeShader, count);
	}

	void RenderPipelineCompute::CreatePipeline()
	{
		auto device = GraphicsContext::GetDevice();

		for (uint32_t i = 0; i < m_count; i++)
		{
			m_shaderResources.emplace_back(m_shader->GetResources());
		}

		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = (uint32_t)m_shaderResources[0].paddedSetLayouts.size();
		layoutInfo.pSetLayouts = m_shaderResources[0].paddedSetLayouts.data();
		layoutInfo.pushConstantRangeCount = (uint32_t)m_shaderResources[0].pushConstantRanges.size();
		layoutInfo.pPushConstantRanges = m_shaderResources[0].pushConstantRanges.data();

		LP_VK_CHECK(vkCreatePipelineLayout(device->GetHandle(), &layoutInfo, nullptr, &m_pipelineLayout));

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = m_pipelineLayout;
		pipelineInfo.flags = 0;
		pipelineInfo.stage = m_shader->GetStageInfos()[0];

		VkPipelineCacheCreateInfo pipelineCacheInfo{};
		pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

		LP_VK_CHECK(vkCreatePipelineCache(device->GetHandle(), &pipelineCacheInfo, nullptr, &m_pipelineCache));
		LP_VK_CHECK(vkCreateComputePipelines(device->GetHandle(), m_pipelineCache, 1, &pipelineInfo, nullptr, &m_pipeline));

		CreateDescriptorPool();
	}

	void RenderPipelineCompute::CreateDescriptorPool()
	{
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = 0;
		poolInfo.maxSets = 100;
		poolInfo.poolSizeCount = (uint32_t)m_shaderResources[0].poolSizes.size();
		poolInfo.pPoolSizes = m_shaderResources[0].poolSizes.data();

		LP_VK_CHECK(vkCreateDescriptorPool(GraphicsContext::GetDevice()->GetHandle(), &poolInfo, nullptr, &m_descriptorPool));
	}

	void RenderPipelineCompute::AllocateAndSetupDescriptorsAndBarriers()
	{
		auto device = GraphicsContext::GetDevice();

		m_bufferBarriers.resize(m_count);
		m_imageBarriers.resize(m_count);

		m_frameDescriptorSets.resize(m_count);
		m_descriptorSetBindings.resize(m_count);
		m_writeDescriptors.resize(m_count);

		for (uint32_t i = 0; i < m_count; i++)
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
						writeDescriptor.pBufferInfo = &shaderResources.uniformBuffersInfos[set].at(binding);
					}
					else if (shaderResources.storageBuffersInfos[set].find(binding) != shaderResources.storageBuffersInfos[set].end())
					{
						writeDescriptor.pBufferInfo = &shaderResources.storageBuffersInfos[set].at(binding).info;

						if (shaderResources.storageBuffersInfos[set].at(binding).writeable)
						{
							auto& bufferBarrier = m_bufferBarriers[i].emplace_back();
							bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
							bufferBarrier.pNext = nullptr;
							bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
							bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
							bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
							bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
							bufferBarrier.buffer = nullptr;
							bufferBarrier.offset = 0;
							bufferBarrier.size = 0;
						}
					}
					else if (shaderResources.storageImagesInfos[set].find(binding) != shaderResources.storageImagesInfos[set].end())
					{
						writeDescriptor.pImageInfo = &shaderResources.storageImagesInfos[set].at(binding).info;

						if (shaderResources.storageImagesInfos[set].at(binding).writeable)
						{
							auto& imageBarrier = m_imageBarriers[i].emplace_back();
							imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
							imageBarrier.pNext = nullptr;
							imageBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
							imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
							imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
							imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
							imageBarrier.image = nullptr;
							imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
							imageBarrier.newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
							imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
							imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
							imageBarrier.subresourceRange.baseMipLevel = 0;
							imageBarrier.subresourceRange.baseArrayLayer = 0;
						}
					}
					else if (shaderResources.imageInfos[set].find(binding) != shaderResources.imageInfos[set].end())
					{
						writeDescriptor.pImageInfo = &shaderResources.imageInfos[set].at(binding).info;
					}

					writeDescriptor.dstSet = m_frameDescriptorSets[i][index];
					m_descriptorSetBindings[i].emplace_back(set);
				}

				index++;
			}
		}
	}

	void RenderPipelineCompute::WriteAndBindDescriptors(VkCommandBuffer cmdBuffer, uint32_t index)
	{
		auto device = GraphicsContext::GetDevice();
		vkUpdateDescriptorSets(device->GetHandle(), (uint32_t)m_writeDescriptors[index].size(), m_writeDescriptors[index].data(), 0, nullptr);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, (uint32_t)m_frameDescriptorSets[index].size(), m_frameDescriptorSets[index].data(), 0, nullptr);
	}
}