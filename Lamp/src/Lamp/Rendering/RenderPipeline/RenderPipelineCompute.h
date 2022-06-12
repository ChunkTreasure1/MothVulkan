#pragma once

#include "Lamp/Core/Base.h"

#include "Lamp/Rendering/Shader/Shader.h"

#include <vulkan/vulkan.h>

namespace Lamp
{
	class Shader;
	class UniformBufferSet;
	class ShaderStorageBufferSet;
	class Texture2D;
	class Image2D;

	class RenderPipelineCompute
	{
	public:
		RenderPipelineCompute(Ref<Shader> computeShader, uint32_t count);
		~RenderPipelineCompute() = default;

		void Bind(VkCommandBuffer commandBuffer, uint32_t frameIndex = 0);
		void InsertBarrier(VkCommandBuffer commandBuffer, uint32_t frameIndex = 0, VkPipelineStageFlags pipelineStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);

		void Dispatch(VkCommandBuffer commandBuffer, uint32_t index, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
		void SetUniformBuffer(Ref<UniformBufferSet> uniformBuffer, uint32_t set, uint32_t binding);
		void SetStorageBuffer(Ref<ShaderStorageBufferSet> storageBuffer, uint32_t set, uint32_t binding, VkAccessFlags accessFlags = VK_ACCESS_SHADER_READ_BIT);
		void SetTexture(Ref<Texture2D> texture, uint32_t set, uint32_t binding);
		void SetImage(Ref<Image2D> image, uint32_t set, uint32_t binding, VkAccessFlags accessFlags = VK_ACCESS_SHADER_READ_BIT, VkImageLayout targetLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		void SetPushConstant(VkCommandBuffer cmdBuffer, uint32_t offset, uint32_t size, const void* data) const;

		static Ref<RenderPipelineCompute> Create(Ref<Shader> computeShader, uint32_t count = 1);

	private:
		void CreatePipeline();
		void CreateDescriptorPool();
		void AllocateAndSetupDescriptorsAndBarriers();
		void WriteAndBindDescriptors(VkCommandBuffer cmdBuffer, uint32_t index = 0);

		Ref<Shader> m_shader;
		uint32_t m_count;
		std::vector<Shader::ShaderResources> m_shaderResources;
		std::vector<std::vector<uint32_t>> m_descriptorSetBindings;
		std::vector<std::vector<VkDescriptorSet>> m_frameDescriptorSets;
		std::vector<std::vector<VkWriteDescriptorSet>> m_writeDescriptors;

		std::unordered_map<uint32_t, std::unordered_map<uint32_t, Ref<ShaderStorageBufferSet>>> m_storageBufferSets; // set -> binding -> storage buffer
		std::unordered_map<uint32_t, std::unordered_map<uint32_t, Ref<Image2D>>> m_images; // set -> binding -> image

		std::vector<std::vector<VkBufferMemoryBarrier>> m_bufferBarriers;
		std::vector<std::vector<VkImageMemoryBarrier>> m_imageBarriers;

		VkPipelineLayout m_pipelineLayout = nullptr;
		VkPipelineCache m_pipelineCache = nullptr;
		VkPipeline m_pipeline = nullptr;

		VkDescriptorPool m_descriptorPool = nullptr;
	};
}