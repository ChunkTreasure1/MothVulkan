#include "lppch.h"
#include "Shader.h"

#include "Lamp/Log/Log.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"

#include "Lamp/Core/Application.h"
#include "Lamp/Core/Window.h"
#include "Lamp/Core/Graphics/Swapchain.h"

#include "Lamp/Rendering/Shader/ShaderUtility.h"
#include "Lamp/Rendering/RenderPipeline/RenderPipeline.h"

#include "ShaderCompiler.h"

#include <shaderc/shaderc.hpp>
#include <file_includer.h>
#include <libshaderc_util/file_finder.h>

#include <spirv_cross/spirv_glsl.hpp>
#include <spirv-tools/libspirv.h>

namespace Lamp
{
	/////ShaderResources/////
	void Shader::ShaderResources::Clear()
	{
		for (const auto& set : paddedSetLayouts)
		{
			vkDestroyDescriptorSetLayout(GraphicsContext::GetDevice()->GetHandle(), set, nullptr);
		}

		paddedSetLayouts.clear();
		realSetLayouts.clear();
		pushConstantRanges.clear();
		uniformBuffersInfos.clear();
		storageBuffersInfos.clear();
		imageInfos.clear();
		writeDescriptors.clear();
	}
	/////////////////////////

	Shader::Shader(const std::string& name, std::initializer_list<std::filesystem::path> paths, bool forceCompile)
		: m_shaderPaths(paths), m_name(name)
	{
		Reload(forceCompile);
		GenerateHash();
	}

	Shader::Shader(const std::string& name, std::vector<std::filesystem::path> paths, bool forceCompile)
		: m_shaderPaths(paths), m_name(name)
	{
		Reload(forceCompile);
		GenerateHash();
	}

	Shader::~Shader()
	{
		Release();
		m_renderPipelineReferences.clear();
	}

	bool Shader::Reload(bool forceCompile)
	{
		Utility::CreateCacheDirectoryIfNeeded();

		m_shaderSources.clear();
		m_pipelineShaderStageInfos.clear();

		LoadShaderFromFiles();

		std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> shaderData;

		if (!CompileOrGetBinary(shaderData, forceCompile))
		{
			return false;
		}

		Release();
		LoadAndCreateShaders(shaderData);
		ReflectAllStages(shaderData);

		for (const auto& pipeline : m_renderPipelineReferences)
		{
			pipeline->Invalidate();
		}

		return true;
	}

	void Shader::AddReference(RenderPipeline* renderPipeline)
	{
		if (auto it = std::find(m_renderPipelineReferences.begin(), m_renderPipelineReferences.end(), renderPipeline); it != m_renderPipelineReferences.end())
		{
			LP_CORE_ERROR("Shader {0} already has a reference to render pipeline {1}!", m_name.c_str(), renderPipeline->GetSpecification().name.c_str());
			return;
		}

		m_renderPipelineReferences.emplace_back(renderPipeline);
	}

	void Shader::RemoveReference(RenderPipeline* renderPipeline)
	{
		auto it = std::find(m_renderPipelineReferences.begin(), m_renderPipelineReferences.end(), renderPipeline);
		if (it == m_renderPipelineReferences.end())
		{
			LP_CORE_ERROR("Reference to render pipeline {0} not found in shader {1}!", renderPipeline->GetSpecification().name.c_str(), m_name.c_str());
			return;
		}

		m_renderPipelineReferences.erase(it);
	}

	Ref<Shader> Shader::Create(const std::string& name, std::initializer_list<std::filesystem::path> paths, bool forceCompile)
	{
		return CreateRef<Shader>(name, paths, forceCompile);
	}

	Ref<Shader> Shader::Create(const std::string& name, std::vector<std::filesystem::path> paths, bool forceCompile)
	{
		return CreateRef<Shader>(name, paths, forceCompile);
	}

	void Shader::LoadShaderFromFiles()
	{
		for (const auto& path : m_shaderPaths)
		{
			VkShaderStageFlagBits stage = Utility::GetShaderStageFromFilename(path.filename().string());
			std::string source = Utility::ReadStringFromFile(path);

			if (m_shaderSources.find(stage) != m_shaderSources.end())
			{
				LP_CORE_ERROR("Multiple shaders of same stage defined in file {0}!", path.string().c_str());
				return;
			}

			m_shaderSources[stage] = source;
		}
	}

	void Shader::Release()
	{
		m_resources.Clear();

		for (const auto& stage : m_pipelineShaderStageInfos)
		{
			vkDestroyShaderModule(GraphicsContext::GetDevice()->GetHandle(), stage.module, nullptr);
		}

		m_pipelineShaderStageInfos.clear();

		m_perStageUBOCount.clear();
		m_perStageSSBOCount.clear();
		m_perStageImageCount.clear();
		m_perStageStorageImageCount.clear();
	}

	bool Shader::CompileOrGetBinary(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outShaderData, bool forceCompile)
	{
		auto cacheDirectory = Utility::GetShaderCacheDirectory();

		uint32_t index = 0;

		for (const auto& [stage, source] : m_shaderSources)
		{
			auto extension = Utility::GetShaderStageCachedFileExtension(stage);
			auto& data = outShaderData[stage];

			std::filesystem::path currentStagePath;
			for (const auto& shaderPath : m_shaderPaths)
			{
				if (stage == Utility::GetShaderStageFromFilename(shaderPath.filename().string()))
				{
					currentStagePath = shaderPath;
					break;
				}
			}

			auto cachedPath = cacheDirectory / (currentStagePath.filename().string() + extension);
			if (!forceCompile)
			{
				std::ifstream file(cachedPath.string(), std::ios::binary | std::ios::in | std::ios::ate);
				if (file.is_open())
				{
					uint64_t size = file.tellg();

					data.resize(size / sizeof(uint32_t));

					file.seekg(0, std::ios::beg);
					file.read((char*)data.data(), size);
					file.close();
				}
			}

			if (data.empty())
			{
				return ShaderCompiler::TryCompile(outShaderData, m_shaderPaths);
			}
		}

		return true;
	}

	void Shader::LoadAndCreateShaders(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData)
	{
		auto device = GraphicsContext::GetDevice();
		m_pipelineShaderStageInfos.clear();

		for (const auto& [stage, data] : shaderData)
		{
			VkShaderModuleCreateInfo moduleInfo{};
			moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleInfo.codeSize = data.size() * sizeof(uint32_t);
			moduleInfo.pCode = data.data();

			VkShaderModule shaderModule;
			LP_VK_CHECK(vkCreateShaderModule(device->GetHandle(), &moduleInfo, nullptr, &shaderModule));

			VkPipelineShaderStageCreateInfo& shaderStage = m_pipelineShaderStageInfos.emplace_back();
			shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStage.stage = stage;
			shaderStage.module = shaderModule;
			shaderStage.pName = "main";
		}
	}

	void Shader::ReflectAllStages(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData)
	{
		m_resources.Clear();
		std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> setLayoutBindings; // set -> bindings

		LP_CORE_INFO("Shader - Reflecting {0}", m_name.c_str());
		for (const auto& [stage, data] : shaderData)
		{
			ReflectStage(stage, data, setLayoutBindings);
		}

		SetupDescriptors(setLayoutBindings);
	}

	void Shader::ReflectStage(VkShaderStageFlagBits stage, const std::vector<uint32_t>& data, std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>>& outSetLayoutBindings)
	{
		auto device = GraphicsContext::GetDevice();

		LP_CORE_INFO("	Reflecting stage {0}", Utility::StageToString(stage).c_str());

		spirv_cross::Compiler compiler(data);
		const auto resources = compiler.get_shader_resources();

		for (const auto& ubo : resources.uniform_buffers)
		{
			auto& bufferType = compiler.get_type(ubo.base_type_id);

			uint32_t binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
			uint32_t set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
			uint32_t size = (uint32_t)compiler.get_declared_struct_size(bufferType);

			auto it = std::find_if(outSetLayoutBindings[set].begin(), outSetLayoutBindings[set].end(), [binding](const VkDescriptorSetLayoutBinding& layoutBinding) { return layoutBinding.binding == binding; });
			if (it == outSetLayoutBindings[set].end())
			{
				VkDescriptorSetLayoutBinding& layoutBinding = outSetLayoutBindings[set].emplace_back();
				layoutBinding.binding = binding;
				layoutBinding.descriptorCount = 1;
				layoutBinding.descriptorType = set == 1 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				layoutBinding.stageFlags = stage;

				UniformBuffer& bufferInfo = m_resources.uniformBuffersInfos[set][binding];
				bufferInfo.info.offset = 0;
				bufferInfo.info.range = size;
				bufferInfo.isDynamic = set == 1;

				if (bufferInfo.isDynamic)
				{
					const uint64_t minUBOAlignment = GraphicsContext::GetDevice()->GetPhysicalDevice()->GetCapabilities().minUBOOffsetAlignment;
					uint32_t dynamicAlignment = size;

					if (minUBOAlignment > 0)
					{
						dynamicAlignment = (uint32_t)Utility::GetAlignedSize((uint64_t)dynamicAlignment, minUBOAlignment);
					}

					bufferInfo.info.range = dynamicAlignment;
					m_resources.dynamicBufferOffsets[set].emplace_back(DynamicOffset{ dynamicAlignment, binding });
				}

				VkWriteDescriptorSet& writeDescriptor = m_resources.writeDescriptors[set][binding];
				writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptor.pNext = nullptr;
				writeDescriptor.dstBinding = binding;
				writeDescriptor.descriptorCount = 1;
				writeDescriptor.descriptorType = set == 1 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

				if (bufferInfo.isDynamic)
				{
					m_perStageDynamicUBOCount[stage].count++;
				}
				else
				{
					m_perStageUBOCount[stage].count++;
				}
			}
			else
			{
				it->stageFlags |= stage;
			}
		}

		for (const auto& ssbo : resources.storage_buffers)
		{
			auto& bufferType = compiler.get_type(ssbo.base_type_id);

			uint32_t binding = compiler.get_decoration(ssbo.id, spv::DecorationBinding);
			uint32_t set = compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
			uint32_t nonWritable = compiler.get_decoration(ssbo.id, spv::DecorationNonWritable);

			uint32_t size = (uint32_t)compiler.get_declared_struct_size(bufferType);

			auto it = std::find_if(outSetLayoutBindings[set].begin(), outSetLayoutBindings[set].end(), [binding](const VkDescriptorSetLayoutBinding& layoutBinding) { return layoutBinding.binding == binding; });
			if (it == outSetLayoutBindings[set].end())
			{
				VkDescriptorSetLayoutBinding& layoutBinding = outSetLayoutBindings[set].emplace_back();
				layoutBinding.binding = binding;
				layoutBinding.descriptorCount = 1;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				layoutBinding.stageFlags = stage;

				ShaderStorageBuffer& bufferInfo = m_resources.storageBuffersInfos[set][binding];
				bufferInfo.info.offset = 0;
				bufferInfo.info.range = size;
				bufferInfo.writeable = !(bool)nonWritable;
				bufferInfo.isDynamic = set == 4;

				if (bufferInfo.isDynamic)
				{
					const uint64_t minSSBOAlignment = GraphicsContext::GetDevice()->GetPhysicalDevice()->GetCapabilities().minSSBOOffsetAlignment;
					uint32_t dynamicAlignment = size;

					if (minSSBOAlignment > 0)
					{
						dynamicAlignment = (uint32_t)Utility::GetAlignedSize((uint64_t)dynamicAlignment, minSSBOAlignment);
					}

					bufferInfo.info.range = dynamicAlignment;
					m_resources.dynamicBufferOffsets[set].emplace_back(DynamicOffset{ dynamicAlignment, binding });
				}

				VkWriteDescriptorSet& writeDescriptor = m_resources.writeDescriptors[set][binding];
				writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptor.pNext = nullptr;
				writeDescriptor.dstBinding = binding;
				writeDescriptor.descriptorCount = 1;
				writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

				if (bufferInfo.isDynamic)
				{
					m_perStageDynamicSSBOCount[stage].count++;
				}
				else
				{
					m_perStageSSBOCount[stage].count++;
				}
			}
			else
			{
				it->stageFlags |= stage;
			}
		}

		for (const auto& pushConst : resources.push_constant_buffers)
		{
			auto& bufferType = compiler.get_type(pushConst.base_type_id);
			uint32_t size = (uint32_t)compiler.get_declared_struct_size(bufferType);
			uint32_t offset = compiler.get_decoration(pushConst.id, spv::DecorationOffset);

			auto it = std::find_if(m_resources.pushConstantRanges.begin(), m_resources.pushConstantRanges.end(), [size, offset](const VkPushConstantRange& range)
				{
					return range.size == size && range.offset == offset;
				});

			if (it == m_resources.pushConstantRanges.end())
			{
				auto& pushConstantRange = m_resources.pushConstantRanges.emplace_back();
				pushConstantRange.offset = offset;
				pushConstantRange.size = size;
				pushConstantRange.stageFlags = stage;
			}
			else
			{
				(*it).stageFlags |= stage;
			}
		}

		for (const auto& image : resources.storage_images)
		{
			uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);
			uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
			uint32_t nonWritable = compiler.get_decoration(image.id, spv::DecorationNonWritable);

			auto it = std::find_if(outSetLayoutBindings[set].begin(), outSetLayoutBindings[set].end(), [binding](const VkDescriptorSetLayoutBinding& layoutBinding) { return layoutBinding.binding == binding; });
			if (it == outSetLayoutBindings[set].end())
			{
				VkDescriptorSetLayoutBinding& layoutBinding = outSetLayoutBindings[set].emplace_back();
				layoutBinding.binding = binding;
				layoutBinding.descriptorCount = 1;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				layoutBinding.stageFlags = stage;

				StorageImage& imageInfo = m_resources.storageImagesInfos[set][binding];
				imageInfo.info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageInfo.writeable = !(bool)nonWritable;

				VkWriteDescriptorSet& writeDescriptor = m_resources.writeDescriptors[set][binding];
				writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptor.pNext = nullptr;
				writeDescriptor.dstBinding = binding;
				writeDescriptor.descriptorCount = 1;
				writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

				m_perStageStorageImageCount[stage].count++;
			}
			else
			{
				it->stageFlags |= stage;
			}
		}

		for (const auto& image : resources.sampled_images)
		{
			uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);
			uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);

			auto it = std::find_if(outSetLayoutBindings[set].begin(), outSetLayoutBindings[set].end(), [binding](const VkDescriptorSetLayoutBinding& layoutBinding) { return layoutBinding.binding == binding; });
			if (it == outSetLayoutBindings[set].end())
			{
				VkDescriptorSetLayoutBinding& layoutBinding = outSetLayoutBindings[set].emplace_back();
				layoutBinding.binding = binding;
				layoutBinding.descriptorCount = 1;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				layoutBinding.stageFlags = stage;

				SampledImage& imageInfo = m_resources.imageInfos[set][binding];

				const auto& type = compiler.get_type(image.type_id);

				switch (type.image.dim)
				{
					case spv::Dim::Dim1D: imageInfo.dimension = ImageDimension::Dim1D; break;
					case spv::Dim::Dim2D: imageInfo.dimension = ImageDimension::Dim2D; break;
					case spv::Dim::Dim3D: imageInfo.dimension = ImageDimension::Dim3D; break;
					case spv::Dim::DimCube: imageInfo.dimension = ImageDimension::DimCube; break;

					default: imageInfo.dimension = ImageDimension::Dim2D; break;
				}

				VkDescriptorImageInfo& descriptorInfo = imageInfo.info;
				descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VkWriteDescriptorSet& writeDescriptor = m_resources.writeDescriptors[set][binding];
				writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptor.pNext = nullptr;
				writeDescriptor.dstBinding = binding;
				writeDescriptor.descriptorCount = 1;
				writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

				m_perStageImageCount[stage].count++;
			}
			else
			{
				it->stageFlags |= stage;
			}
		}

		LP_CORE_INFO("		Uniform Buffers: {0}", m_perStageUBOCount[stage].count);
		LP_CORE_INFO("		Dynamic Uniform Buffers: {0}", m_perStageDynamicUBOCount[stage].count);
		LP_CORE_INFO("		Shader Storage Buffers: {0}", m_perStageSSBOCount[stage].count);
		LP_CORE_INFO("		Dynamic Shader Storage Buffers: {0}", m_perStageDynamicSSBOCount[stage].count);
		LP_CORE_INFO("		Sampled Images: {0}", m_perStageImageCount[stage].count);
		LP_CORE_INFO("		Storage Images: {0}", m_perStageStorageImageCount[stage].count);
	}

	void Shader::SetupDescriptors(const std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>>& setLayoutBindings)
	{
		int32_t lastSet = -1;

		for (auto& [set, offsets] : m_resources.dynamicBufferOffsets)
		{
			std::sort(offsets.begin(), offsets.end(), [](const DynamicOffset& lhs, const DynamicOffset& rhs)
				{
					return lhs.binding < rhs.binding;
				});
		}

		for (const auto& [set, bindings] : setLayoutBindings)
		{
			while ((int32_t)set > lastSet + 1)
			{
				VkDescriptorSetLayoutCreateInfo layoutInfo{};
				layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layoutInfo.pNext = nullptr;
				layoutInfo.bindingCount = 0;
				layoutInfo.pBindings = nullptr;

				vkCreateDescriptorSetLayout(GraphicsContext::GetDevice()->GetHandle(), &layoutInfo, nullptr, &m_resources.paddedSetLayouts.emplace_back());
				lastSet++;
			}

			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.pNext = nullptr;
			layoutInfo.bindingCount = (uint32_t)bindings.size();
			layoutInfo.pBindings = bindings.data();

			vkCreateDescriptorSetLayout(GraphicsContext::GetDevice()->GetHandle(), &layoutInfo, nullptr, &m_resources.paddedSetLayouts.emplace_back());
			m_resources.realSetLayouts.emplace_back(m_resources.paddedSetLayouts.back());
			lastSet = set;
		}

		VkDescriptorSetAllocateInfo& allocInfo = m_resources.setAllocInfo;
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = (uint32_t)m_resources.realSetLayouts.size();
		allocInfo.pSetLayouts = m_resources.realSetLayouts.data();
		allocInfo.pNext = nullptr;

		const uint32_t framesInFlight = Application::Get().GetWindow()->GetSwapchain().GetFramesInFlight();

		uint32_t uboCount = 0;
		uint32_t dynamicUBOCount = 0;
		uint32_t ssboCount = 0;
		uint32_t storageImageCount = 0;
		uint32_t dynamicSSBOCount = 0;
		uint32_t imageCount = 0;

		std::for_each(m_perStageUBOCount.begin(), m_perStageUBOCount.end(), [&](auto pair) { uboCount += pair.second.count; });
		std::for_each(m_perStageDynamicUBOCount.begin(), m_perStageDynamicUBOCount.end(), [&](auto pair) { dynamicUBOCount += pair.second.count; });
		std::for_each(m_perStageSSBOCount.begin(), m_perStageSSBOCount.end(), [&](auto pair) { ssboCount += pair.second.count; });
		std::for_each(m_perStageDynamicSSBOCount.begin(), m_perStageDynamicSSBOCount.end(), [&](auto pair) { dynamicSSBOCount += pair.second.count; });
		std::for_each(m_perStageStorageImageCount.begin(), m_perStageStorageImageCount.end(), [&](auto pair) { storageImageCount += pair.second.count; });
		std::for_each(m_perStageImageCount.begin(), m_perStageImageCount.end(), [&](auto pair) { imageCount += pair.second.count; });

		if (uboCount > 0)
		{
			m_resources.poolSizes.emplace_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uboCount * framesInFlight);
		}

		if (dynamicUBOCount > 0)
		{
			m_resources.poolSizes.emplace_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, dynamicUBOCount * framesInFlight);
		}

		if (ssboCount > 0)
		{
			m_resources.poolSizes.emplace_back(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, ssboCount * framesInFlight);
		}

		if (dynamicSSBOCount > 0)
		{
			m_resources.poolSizes.emplace_back(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, dynamicSSBOCount * framesInFlight);
		}

		if (storageImageCount > 0)
		{
			m_resources.poolSizes.emplace_back(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, storageImageCount * framesInFlight);
		}

		if (imageCount > 0)
		{
			m_resources.poolSizes.emplace_back(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount * framesInFlight);
		}
	}

	void Shader::GenerateHash()
	{
		size_t hash = std::hash<std::string>()(m_name);
		for (const auto& path : m_shaderPaths)
		{
			size_t pathHash = std::filesystem::hash_value(path);
			hash = Utility::HashCombine(hash, pathHash);
		}

		m_hash = hash;
	}
}