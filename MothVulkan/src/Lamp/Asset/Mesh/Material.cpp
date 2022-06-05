#include "lppch.h"
#include "Material.h"

#include "Lamp/Core/Application.h"
#include "Lamp/Core/Window.h"
#include "Lamp/Core/Graphics/Swapchain.h"

#include "Lamp/Log/Log.h"
#include "Lamp/Asset/Mesh/MaterialInstance.h"

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
	}

	Material::~Material()
	{
		if (m_renderPipeline)
		{
			m_renderPipeline->RemoveReference(this);
		}
	}

	void Material::SetTexture(uint32_t binding, Ref<Texture2D> texture)
	{
		m_textures[binding] = texture;
		UpdateTextureWriteDescriptor(binding);
	}

	void Material::AddReference(MaterialInstance* materialInstance)
	{
		m_materialInstanceReferences.emplace_back(materialInstance);
	}

	void Material::RemoveReference(MaterialInstance* materialInstance)
	{
		auto it = std::find(m_materialInstanceReferences.begin(), m_materialInstanceReferences.end(), materialInstance);
		if (it != m_materialInstanceReferences.end())
		{
			m_materialInstanceReferences.erase(it);
		}
		else
		{
			LP_CORE_ERROR("Material instance not found in material instance references");
		}
	}

	void Material::Invalidate()
	{
		m_shaderResources.clear();
		SetupMaterialFromPipeline();

		for (const auto& materialInstance : m_materialInstanceReferences)
		{
			materialInstance->Invalidate();
		}
	}

	Ref<Material> Material::Create(const std::string& name, uint32_t index, Ref<RenderPipeline> renderPipeline)
	{
		return CreateRef<Material>(name, index, renderPipeline);
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

		for (auto ref : m_materialInstanceReferences)
		{
			ref->Invalidate();
		}
	}
}
