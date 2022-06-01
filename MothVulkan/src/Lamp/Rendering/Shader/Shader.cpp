#include "lppch.h"
#include "Shader.h"

#include "Lamp/Log/Log.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"

#include "Lamp/Rendering/Shader/ShaderUtility.h"

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
		for (const auto& set : setLayouts)
		{
			vkDestroyDescriptorSetLayout(GraphicsContext::GetDevice()->GetHandle(), set, nullptr);
		}

		setLayouts.clear();
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
	}

	Shader::Shader(const std::string& name, std::vector<std::filesystem::path> paths, bool forceCompile)
		: m_shaderPaths(paths), m_name(name)
	{
		Reload(forceCompile);
	}

	Shader::~Shader()
	{
		Release();
	}

	void Shader::Reload(bool forceCompile)
	{
		Utility::CreateCacheDirectoryIfNeeded();

		Release();
		LoadShaderFromFiles();

		std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> shaderData;
		CompileOrGetBinary(shaderData, forceCompile);
		LoadAndCreateShaders(shaderData);
		ReflectAllStages(shaderData);
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
			VkShaderStageFlagBits stage = Utility::GetShaderStageFromExtension(path.extension().string());
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
	}

	void Shader::CompileOrGetBinary(std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outShaderData, bool forceCompile)
	{
		auto cacheDirectory = Utility::GetShaderCacheDirectory();

		uint32_t index = 0;

		for (const auto& [stage, source] : m_shaderSources)
		{
			auto extension = Utility::GetShaderStageCachedFileExtension(stage);

			auto& data = outShaderData[stage];

			if (!forceCompile)
			{
				auto cachedPath = cacheDirectory / (path.filename().string() + extension);

				std::ifstream file(cachedPath.string(), std::ios::binary | std::ios::in | std::ios::ate);
				if (file.is_open())
				{
					uint64_t size = file.tellg();

					data.resize(size / sizeof(uint32_t));

					file.seekg(0, std::ios::beg);
					file.read((char*)data.data(), size);
					file.close();
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

		for (const auto& [stage, data] : shaderData)
		{
			ReflectStage(stage, data, setLayoutBindings);
		}

		SetupDescriptors(setLayoutBindings);
	}

	void Shader::ReflectStage(VkShaderStageFlagBits stage, const std::vector<uint32_t>& data, std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>>& outSetLayoutBindings)
	{
		auto device = GraphicsContext::GetDevice();

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
			}
			else
			{
				it->stageFlags |= stage;
			}
		}

		for (const auto& pushConst : resources.push_constant_buffers)
		{
			auto& bufferType = compiler.get_type(pushConst.base_type_id);
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
			}
			else
			{
				it->stageFlags |= stage;
			}
		}
	}

	void Shader::SetupDescriptors(const std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>>& setLayoutBindings)
	{
		for (const auto& [set, bindings] : setLayoutBindings)
		{
			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.pNext = nullptr;
			layoutInfo.bindingCount = (uint32_t)bindings.size();
			layoutInfo.pBindings = bindings.data();

			vkCreateDescriptorSetLayout(GraphicsContext::GetDevice()->GetHandle(), &layoutInfo, nullptr, &m_resources.setLayouts.emplace_back());
		}

		VkDescriptorSetAllocateInfo& allocInfo = m_resources.setAllocInfo;
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = (uint32_t)m_resources.setLayouts.size();
		allocInfo.pSetLayouts = m_resources.setLayouts.data();
		allocInfo.pNext = nullptr;
	}
}