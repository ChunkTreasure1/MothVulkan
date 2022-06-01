#pragma once

#include "Lamp/Asset/Asset.h"

#include <vulkan/vulkan.h>

#include <filesystem>
#include <map>

namespace Lamp
{
	class Shader : public Asset
	{
	public:
		struct ShaderResources
		{
			std::vector<VkDescriptorSetLayout> setLayouts;
			std::map<VkShaderStageFlagBits, VkPushConstantRange> pushConstantRanges;
			
			std::map<uint32_t, std::map<uint32_t, VkDescriptorBufferInfo>> uniformBuffersInfos; // set -> infos
			std::map<uint32_t, std::map<uint32_t, VkDescriptorBufferInfo>> storageBuffersInfos; // set -> infos
			std::map<uint32_t, std::map<uint32_t, VkDescriptorImageInfo>> imageInfos; // set -> infos
			
			std::map<uint32_t, std::map<uint32_t, VkWriteDescriptorSet>> writeDescriptors; // set -> binding -> write

			VkDescriptorSetAllocateInfo setAllocInfo{};
			
			void Clear();
		};

		Shader(const std::string& name, std::initializer_list<std::filesystem::path> paths, bool forceCompile);
		Shader(const std::string& name, std::vector<std::filesystem::path> paths, bool forceCompile);
		Shader() = default;
		~Shader();

		void Reload(bool forceCompile);
	
		inline const std::vector<VkPipelineShaderStageCreateInfo>& GetStageInfos() const { return m_pipelineShaderStageInfos; }
		inline const ShaderResources& GetResources() const { return m_resources; }

		static AssetType GetStaticType() { return AssetType::Shader; }
		AssetType GetType() override { return GetStaticType(); }

		static Ref<Shader> Create(const std::string& name, std::initializer_list<std::filesystem::path> paths, bool forceCompile = false);
		static Ref<Shader> Create(const std::string& name, std::vector<std::filesystem::path> paths, bool forceCompile = false);
	
	private:
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

		ShaderResources m_resources;
		std::string m_name;
	};
}