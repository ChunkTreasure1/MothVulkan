#pragma once

#include "Lamp/Core/Base.h"

#include "Lamp/Rendering/Shader/Shader.h"

#include <vulkan/vulkan.h>

namespace Lamp
{
	class Shader;
	class RenderPipelineCompute
	{
	public:
		RenderPipelineCompute(Ref<Shader> computeShader);
		~RenderPipelineCompute() = default;

		void Begin(VkCommandBuffer commandBuffer);
		void End();

		void Dispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

		static Ref<RenderPipelineCompute> Create(Ref<Shader> computeShader);

	private:
		void CreatePipeline();
		void CreateDescriptorPool();

		Ref<Shader> m_shader;
		Shader::ShaderResources m_shaderResources;

		VkPipelineLayout m_pipelineLayout = nullptr;
		VkPipelineCache m_pipelineCache = nullptr;
		VkPipeline m_pipeline = nullptr;

		VkDescriptorPool m_descriptorPool = nullptr;
	};
}