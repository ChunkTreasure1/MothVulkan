#pragma once

#include "Lamp/Rendering/Texture/ImageCommon.h"

#include <vulkan/vulkan.h>

namespace Lamp
{
	class SamplerLibrary
	{
	public:
		static void Shutdown();
		static void Add(TextureFilter minFilter, TextureFilter magFilter, TextureFilter mipMode, TextureWrap wrapMode, CompareOperator compareOp, AniostopyLevel anisoLevel = AniostopyLevel::None);
		static VkSampler Get(TextureFilter minFilter, TextureFilter magFilter, TextureFilter mipMode, TextureWrap wrapMode, CompareOperator compareOp = CompareOperator::None, AniostopyLevel anisoLevel = AniostopyLevel::None);

	private:
		SamplerLibrary() = default;

		static const size_t GetHashFromSamplerSettings(TextureFilter minFilter, TextureFilter magFilter, TextureFilter mipMode, TextureWrap wrapMode, CompareOperator compareOp, AniostopyLevel anisoLevel = AniostopyLevel::None);

		inline static std::unordered_map<size_t, VkSampler> m_samplers; // Hash -> Sampler
	};
}