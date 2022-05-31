#pragma once

#include "Lamp/Core/Base.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

struct GLFWwindow;

namespace Lamp
{
	struct ApplicationInfo
	{
		ApplicationInfo(const std::string& aTitle = "Lamp", uint32_t aWidth = 1280, uint32_t aHeight = 720, bool aUseVSync = true, bool aEnableImGui = true)
			: title(aTitle), width(aWidth), height(aHeight), useVSync(aUseVSync), enableImGui(aEnableImGui)
		{ }

		std::string title;
		uint32_t width;
		uint32_t height;
		bool useVSync;
		bool enableImGui;
	};

	struct MeshPushConstants
	{
		glm::vec4 data;
		glm::mat4 transform;
	};

	struct CameraData
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewProj;
	};

	struct ObjectData
	{
		glm::mat4 transform;
	};

	class Window;
	class Mesh;
	class UniformBufferSet;
	class ShaderStorageBufferSet;
	class Texture2D;

	class AssetManager;

	class Application
	{
	public:
		Application(const ApplicationInfo& info);
		virtual ~Application();

		void Run();
		void Shutdown();

		inline static Application& Get() { return *s_instance; }

	private:
		void CreatePipeline();
		void CreateDescriptors();

		const std::vector<const char*> m_validationLayers = { "VK_LAYER_KHRONOS_validation" };

		bool m_isRunning = true;

		Ref<Window> m_window;
		Ref<AssetManager> m_assetManager;

		ApplicationInfo m_applicationInfo;
		inline static Application* s_instance;


		//////Pipeline/////
		VkPipelineLayout m_pipelineLayout;
		VkPipeline m_pipeline;

		uint32_t m_frameNumber = 0;
		///////////////////
		
		/////Assets/////
		Ref<Mesh> m_mesh;
		Ref<Texture2D> m_texture;
		////////////////
		
		/////Descriptors/////
		VkDescriptorSetLayout m_globalSetLayout;
		VkDescriptorSetLayout m_objectSetLayout;

		VkDescriptorPool m_descriptorPool;

		std::vector<VkDescriptorSet> m_globalDescriptorSets; // One per frame
		std::vector<VkDescriptorSet> m_objectDescriptorSets; // One per frame
		
		Ref<UniformBufferSet> m_uniformBufferSet;
		Ref<ShaderStorageBufferSet> m_objectBufferSet;
		/////////////////////
	};

	static Application* CreateApplication();
}