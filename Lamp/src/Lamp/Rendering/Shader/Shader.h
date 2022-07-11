#pragma once

#include "Lamp/Asset/Asset.h"
#include "Lamp/Rendering/Texture/ImageCommon.h"

#include <vulkan/vulkan.h>

#include <filesystem>
#include <map>

namespace Lamp
{
	// Descriptor sets:
	// 0 - Per frame
	// 1 - Per pass
	// 2 - Per object -- Unused for now
	// 3 - Per material -- All textures for now

	enum class DescriptorSetType : uint32_t
	{
		PerFrame = 0,
		PerPass = 1,
		PerObject = 2,
		PerMaterial = 3
	};

	class RenderPipeline;
	class Shader : public Asset
	{
	public:
		struct ShaderStorageBuffer
		{
			VkDescriptorBufferInfo info;
			bool writeable = true;
		};

		struct StorageImage
		{
			VkDescriptorImageInfo info;
			bool writeable = true;
		};

		struct SampledImage
		{
			VkDescriptorImageInfo info;
			ImageDimension dimension;
		};

		struct ShaderResources
		{
			std::unordered_map<uint32_t, std::string> shaderTextureDefinitions; // binding -> name

			std::vector<VkDescriptorSetLayout> paddedSetLayouts;
			std::vector<VkDescriptorSetLayout> realSetLayouts;

			std::vector<VkPushConstantRange> pushConstantRanges;
			std::vector<VkDescriptorPoolSize> poolSizes;
			
			std::map<uint32_t, std::map<uint32_t, VkDescriptorBufferInfo>> uniformBuffersInfos; // set -> binding -> infos
			std::map<uint32_t, std::map<uint32_t, ShaderStorageBuffer>> storageBuffersInfos; // set -> binding -> infos
			std::map<uint32_t, std::map<uint32_t, StorageImage>> storageImagesInfos; // set -> binding -> infos
			std::map<uint32_t, std::map<uint32_t, SampledImage>> imageInfos; // set -> binding -> infos
			std::map<uint32_t, std::map<uint32_t, VkWriteDescriptorSet>> writeDescriptors; // set -> binding -> write

			VkDescriptorSetAllocateInfo setAllocInfo{};
			
			void Clear();
		};

		Shader(const std::string& name, std::initializer_list<std::filesystem::path> paths, bool forceCompile);
		Shader(const std::string& name, std::vector<std::filesystem::path> paths, bool forceCompile);
		Shader() = default;
		~Shader();

		void Reload(bool forceCompile);
		void AddReference(RenderPipeline* renderPipeline);
		void RemoveReference(RenderPipeline* renderPipeline);
	
		inline const std::vector<VkPipelineShaderStageCreateInfo>& GetStageInfos() const { return m_pipelineShaderStageInfos; }
		inline const ShaderResources& GetResources() const { return m_resources; }
		inline const std::string& GetName() const { return m_name; }
		inline const size_t GetHash() const { return m_hash; }

		static AssetType GetStaticType() { return AssetType::Shader; }
		AssetType GetType() override { return GetStaticType(); }

		static Ref<Shader> Create(const std::string& name, std::initializer_list<std::filesystem::path> paths, bool forceCompile = false);
		static Ref<Shader> Create(const std::string& name, std::vector<std::filesystem::path> paths, bool forceCompile = false);
	
	private:
		struct TypeCount
		{
			uint32_t count = 0;
		};

		void GenerateHash();
		void LoadShaderFromFiles();
		void Release();

		void CompileOrGetBinary(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outShaderData, bool forceCompile);
		void LoadAndCreateShaders(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData);
		void ReflectAllStages(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData);
		void ReflectStage(VkShaderStageFlagBits stage, const std::vector<uint32_t>& data, std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>>& outSetLayoutBindings);
		
		void SetupDescriptors(const std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>>& setLayoutBindings);

		std::unordered_map<VkShaderStageFlagBits, std::string> m_shaderSources;
		std::vector<VkPipelineShaderStageCreateInfo> m_pipelineShaderStageInfos;
		std::vector<std::filesystem::path> m_shaderPaths;
		std::vector<RenderPipeline*> m_renderPipelineReferences;

		ShaderResources m_resources;
		std::string m_name;

		std::unordered_map<VkShaderStageFlagBits, TypeCount> m_perStageUBOCount;
		std::unordered_map<VkShaderStageFlagBits, TypeCount> m_perStageSSBOCount;
		std::unordered_map<VkShaderStageFlagBits, TypeCount> m_perStageStorageImageCount;
		std::unordered_map<VkShaderStageFlagBits, TypeCount> m_perStageImageCount;

		size_t m_hash;
	};
}