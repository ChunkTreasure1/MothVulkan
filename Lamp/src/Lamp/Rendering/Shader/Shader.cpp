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
	}

	Shader::~Shader()
	{
		Release();
		m_renderPipelineReferences.clear();
	}

	void Shader::Reload(bool forceCompile)
	{
		Utility::CreateCacheDirectoryIfNeeded();
		Release();

		m_shaderSources.clear();
		m_pipelineShaderStageInfos.clear();

		LoadShaderFromFiles();

		std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> shaderData;
		CompileOrGetBinary(shaderData, forceCompile);
		LoadAndCreateShaders(shaderData);
		ReflectAllStages(shaderData);

		for (const auto& pipeline : m_renderPipelineReferences)
		{
			pipeline->Invalidate();
		}
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
		m_uboCount = 0;
		m_ssboCount = 0;
		m_imageCount = 0;
	}

	void Shader::CompileOrGetBinary(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outShaderData, bool forceCompile)
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
				shaderc::Compiler compiler;
				shaderc::CompileOptions compileOptions;

				shaderc_util::FileFinder fileFinder;
				fileFinder.search_path().emplace_back("Engine/Shaders/");

				compileOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
				compileOptions.SetWarningsAsErrors();
				compileOptions.SetIncluder(std::make_unique<glslc::FileIncluder>(&fileFinder));
				compileOptions.SetOptimizationLevel(shaderc_optimization_level_performance);

#ifdef LP_ENABLE_SHADER_DEBUG
				compileOptions.SetGenerateDebugInfo();
#endif

				const auto& currentPath = m_shaderPaths[index];

				shaderc::PreprocessedSourceCompilationResult preProcessResult = compiler.PreprocessGlsl(source, Utility::VulkanToShaderCStage(stage), currentPath.string().c_str(), compileOptions);
				if (preProcessResult.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					LP_CORE_ERROR("Failed to preprocess shader {0}!", currentPath.string().c_str());
					LP_CORE_ERROR("{0}", preProcessResult.GetErrorMessage().c_str());
					LP_CORE_ASSERT(false, "Failed to preprocess shader!");
				}

				std::string proccessedSource = std::string(preProcessResult.cbegin(), preProcessResult.cend());

				// Compile shader
				{
					shaderc::SpvCompilationResult compileResult = compiler.CompileGlslToSpv(proccessedSource, Utility::VulkanToShaderCStage(stage), currentPath.string().c_str());
					if (compileResult.GetCompilationStatus() != shaderc_compilation_status_success)
					{
						LP_CORE_ERROR("Failed to compile shader {0}!", currentPath.string().c_str());
						LP_CORE_ERROR(compileResult.GetErrorMessage().c_str());
						LP_CORE_ASSERT(false, "Shader compilation failed!");
					}

					const uint8_t* begin = (const uint8_t*)compileResult.cbegin();
					const uint8_t* end = (const uint8_t*)compileResult.cend();
					const ptrdiff_t size = end - begin;

					data = std::vector<uint32_t>(compileResult.cbegin(), compileResult.cend());
				}

				// Cache shader
				{
					std::ofstream output(cachedPath, std::ios::binary | std::ios::out);
					if (!output.is_open())
					{
						LP_CORE_ERROR("Failed to open file {0} for writing!", cachedPath.string().c_str());
						LP_CORE_ASSERT(false, "Failed to open file for writing!");
					}

					output.write((const char*)data.data(), data.size() * sizeof(uint32_t));
					output.close();
				}
			}

			index++;
		}
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
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				layoutBinding.stageFlags = stage;

				VkDescriptorBufferInfo& bufferInfo = m_resources.uniformBuffersInfos[set][binding];
				bufferInfo.offset = 0;
				bufferInfo.range = size;

				VkWriteDescriptorSet& writeDescriptor = m_resources.writeDescriptors[set][binding];
				writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptor.pNext = nullptr;
				writeDescriptor.dstBinding = binding;
				writeDescriptor.descriptorCount = 1;
				writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

				m_uboCount++;
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
			uint32_t size = (uint32_t)compiler.get_declared_struct_size(bufferType);

			auto it = std::find_if(outSetLayoutBindings[set].begin(), outSetLayoutBindings[set].end(), [binding](const VkDescriptorSetLayoutBinding& layoutBinding) { return layoutBinding.binding == binding; });
			if (it == outSetLayoutBindings[set].end())
			{
				VkDescriptorSetLayoutBinding& layoutBinding = outSetLayoutBindings[set].emplace_back();
				layoutBinding.binding = binding;
				layoutBinding.descriptorCount = 1;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				layoutBinding.stageFlags = stage;

				VkDescriptorBufferInfo& bufferInfo = m_resources.storageBuffersInfos[set][binding];
				bufferInfo.offset = 0;
				bufferInfo.range = size;

				VkWriteDescriptorSet& writeDescriptor = m_resources.writeDescriptors[set][binding];
				writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptor.pNext = nullptr;
				writeDescriptor.dstBinding = binding;
				writeDescriptor.descriptorCount = 1;
				writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

				m_ssboCount++;
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

			auto it = std::find_if(outSetLayoutBindings[set].begin(), outSetLayoutBindings[set].end(), [binding](const VkDescriptorSetLayoutBinding& layoutBinding) { return layoutBinding.binding == binding; });
			if (it == outSetLayoutBindings[set].end())
			{
				VkDescriptorSetLayoutBinding& layoutBinding = outSetLayoutBindings[set].emplace_back();
				layoutBinding.binding = binding;
				layoutBinding.descriptorCount = 1;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				layoutBinding.stageFlags = stage;

				VkDescriptorImageInfo& imageInfo = m_resources.storageImagesInfos[set][binding];
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

				VkWriteDescriptorSet& writeDescriptor = m_resources.writeDescriptors[set][binding];
				writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptor.pNext = nullptr;
				writeDescriptor.dstBinding = binding;
				writeDescriptor.descriptorCount = 1;
				writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

				m_storageImageCount++;
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

				VkDescriptorImageInfo& imageInfo = m_resources.imageInfos[set][binding];
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VkWriteDescriptorSet& writeDescriptor = m_resources.writeDescriptors[set][binding];
				writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptor.pNext = nullptr;
				writeDescriptor.dstBinding = binding;
				writeDescriptor.descriptorCount = 1;
				writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

				m_imageCount++;
			}
			else
			{
				it->stageFlags |= stage;
			}
		}

		LP_CORE_INFO("		Uniform Buffers: {0}", m_uboCount);
		LP_CORE_INFO("		Shader Storage Buffers: {0}", m_ssboCount);
		LP_CORE_INFO("		Sampled Images: {0}", m_imageCount);
		LP_CORE_INFO("		Storage Images: {0}", m_storageImageCount);
	}

	void Shader::SetupDescriptors(const std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>>& setLayoutBindings)
	{
		uint32_t lastSet = 0;

		for (const auto& [set, bindings] : setLayoutBindings)
		{
			if (set > lastSet + 1)
			{
				VkDescriptorSetLayoutCreateInfo layoutInfo{};
				layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layoutInfo.pNext = nullptr;
				layoutInfo.bindingCount = 0;
				layoutInfo.pBindings = nullptr;

				vkCreateDescriptorSetLayout(GraphicsContext::GetDevice()->GetHandle(), &layoutInfo, nullptr, &m_resources.paddedSetLayouts.emplace_back());
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

		if (m_uboCount > 0)
		{
			m_resources.poolSizes.emplace_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_uboCount * framesInFlight);
		}

		if (m_ssboCount > 0)
		{
			m_resources.poolSizes.emplace_back(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_ssboCount * framesInFlight);
		}

		if (m_storageImageCount > 0)
		{
			m_resources.poolSizes.emplace_back(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_storageImageCount * framesInFlight);
		}

		if (m_imageCount > 0)
		{
			m_resources.poolSizes.emplace_back(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_imageCount * framesInFlight);
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