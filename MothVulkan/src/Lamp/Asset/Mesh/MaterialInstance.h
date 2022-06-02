#pragma once

#include "Lamp/Core/Base.h"

#include <vulkan/vulkan.h>

namespace Lamp
{
	class Material;
	class MaterialInstance
	{
	public:
		MaterialInstance(Ref<Material> sharedMaterial);
		~MaterialInstance();

		void Bind(VkCommandBuffer cmdBuffer, uint32_t frame);
		void ReconstructMaterial();

		inline static Ref<MaterialInstance> Create(Ref<Material> sharedMaterial) { return CreateRef<MaterialInstance>(sharedMaterial); }

	private:
		void CreateDescriptorPool();
		void AllocateAndSetupDescriptorSets();

		std::vector<std::vector<uint32_t>> m_descriptorSetBindings; // maps descriptor set vector index to descriptor set binding
		std::vector<std::vector<VkDescriptorSet>> m_frameDescriptorSets; // frame -> index -> descriptor set
		std::vector<std::vector<VkWriteDescriptorSet>> m_writeDescriptors;

		VkDescriptorPool m_descriptorPool;

		Ref<Material> m_sharedMaterial;
	};
}