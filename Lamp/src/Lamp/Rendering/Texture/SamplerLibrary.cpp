#include "lppch.h"
#include "SamplerLibrary.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"

#include "Lamp/Rendering/Renderer.h"
#include "Lamp/Rendering/Shader/ShaderUtility.h"

#include "Lamp/Utility/ImageUtility.h"

namespace Lamp
{
	void SamplerLibrary::Shutdown()
	{
		auto device = GraphicsContext::GetDevice();

		for (const auto& [hash, sampler] : m_samplers)
		{
			vkDestroySampler(device->GetHandle(), sampler, nullptr);
		}

		m_samplers.clear();
	}

	void SamplerLibrary::Add(TextureFilter minFilter, TextureFilter magFilter, TextureFilter mipMode, TextureWrap wrapMode, CompareOperator compareOp, AniostopyLevel anisoLevel)
	{
		const size_t hash = GetHashFromSamplerSettings(minFilter, magFilter, mipMode, wrapMode, compareOp, anisoLevel);
	
		if (auto it = m_samplers.find(hash); it != m_samplers.end())
		{
			LP_CORE_WARN("Sampler already exists! Skipping!");
			return;
		}

		VkSamplerCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.anisotropyEnable = anisoLevel != AniostopyLevel::None ? true : false;
		info.maxAnisotropy = (float)anisoLevel;
		
		info.magFilter = Utility::LampToVulkanFilter(magFilter);
		info.minFilter = Utility::LampToVulkanFilter(minFilter);
		info.mipmapMode = Utility::LampToVulkanMipMapMode(mipMode);
	
		info.addressModeU = Utility::LampToVulkanWrap(wrapMode);
		info.addressModeV = Utility::LampToVulkanWrap(wrapMode);
		info.addressModeW = Utility::LampToVulkanWrap(wrapMode);
	
		info.mipLodBias = 0.f;
		info.minLod = 0.f;
		info.maxLod = FLT_MAX;

		info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		info.compareEnable = compareOp != CompareOperator::None ? true : false;
		info.compareOp = Utility::LampToVulkanCompareOp(compareOp);

		auto device = GraphicsContext::GetDevice();

		VkSampler sampler = nullptr;
		LP_VK_CHECK(vkCreateSampler(device->GetHandle(), &info, nullptr, &sampler));

		m_samplers.emplace(hash, sampler);
	}
	
	VkSampler SamplerLibrary::Get(TextureFilter minFilter, TextureFilter magFilter, TextureFilter mipMode, TextureWrap wrapMode, CompareOperator compareOp, AniostopyLevel anisoLevel)
	{
		const size_t hash = GetHashFromSamplerSettings(minFilter, magFilter, mipMode, wrapMode, compareOp, anisoLevel);
		
		auto it = m_samplers.find(hash);
		if (it == m_samplers.end())
		{
			LP_CORE_ERROR("Unable to find sampler!");
			return nullptr;
		}

		return it->second;
	}
	
	const size_t SamplerLibrary::GetHashFromSamplerSettings(TextureFilter minFilter, TextureFilter magFilter, TextureFilter mipMode, TextureWrap wrapMode, CompareOperator compareOp, AniostopyLevel anisoLevel)
	{
		size_t hash = std::hash<uint32_t>()((uint32_t)minFilter);
		hash = Utility::HashCombine(hash, std::hash<uint32_t>()((uint32_t)minFilter));
		hash = Utility::HashCombine(hash, std::hash<uint32_t>()((uint32_t)magFilter));
		hash = Utility::HashCombine(hash, std::hash<uint32_t>()((uint32_t)mipMode));
		hash = Utility::HashCombine(hash, std::hash<uint32_t>()((uint32_t)wrapMode));
		hash = Utility::HashCombine(hash, std::hash<uint32_t>()((uint32_t)compareOp));
		hash = Utility::HashCombine(hash, std::hash<uint32_t>()((uint32_t)anisoLevel));

		return hash;
	}
}