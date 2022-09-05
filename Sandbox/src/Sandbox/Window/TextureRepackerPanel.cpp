#include "sbpch.h"
#include "TextureRepackerPanel.h"

#include <Lamp/Core/Graphics/GraphicsContext.h>
#include <Lamp/Core/Graphics/GraphicsDevice.h>
#include <Lamp/Asset/AssetManager.h>

#include <Lamp/Rendering/Texture/Texture2D.h>
#include <Lamp/Rendering/Texture/Image2D.h>
#include <Lamp/Rendering/RenderPipeline/RenderPipelineCompute.h>
#include <Lamp/Rendering/Shader/ShaderRegistry.h>

#include <Lamp/Utility/FileSystem.h>

TextureRepackerPanel::TextureRepackerPanel()
	: EditorWindow("Texture Repacker")
{}

void TextureRepackerPanel::UpdateMainContent()
{
	if (ImGui::Button("Open normals folder"))
	{
		m_normalPaths = FileSystem::OpenFolder();
	}

	if (ImGui::Button("Open metallics folder"))
	{
		m_metallicPaths = FileSystem::OpenFolder();
	}

	if (ImGui::Button("Open roughness folder"))
	{
		m_rougnessPaths = FileSystem::OpenFolder();
	}

	static bool repack = false;

	if (ImGui::Button("Convert"))
	{
		//for (const auto& file : std::filesystem::directory_iterator(m_currentTexturesPath))
		//{
		//	if (!file.is_directory())
		//	{
		//		RepackTexture(file.path());
		//	}
		//}
	}
}

void TextureRepackerPanel::RepackTexture(const std::filesystem::path& path)
{
	Ref<Lamp::Texture2D> inputTexture = Lamp::AssetManager::GetAsset<Lamp::Texture2D>(path);
	if (inputTexture && inputTexture->IsValid())
	{
		const uint32_t width = inputTexture->GetWidth();
		const uint32_t height = inputTexture->GetHeight();

		Lamp::ImageSpecification imageSpec{};
		imageSpec.format = Lamp::ImageFormat::RGBA;
		imageSpec.width = width;
		imageSpec.height = height;
		imageSpec.usage = Lamp::ImageUsage::Storage;
		imageSpec.filter = Lamp::TextureFilter::Linear;
		imageSpec.copyable = true;
	
		Ref<Lamp::Image2D> image = Lamp::Image2D::Create(imageSpec);
		Ref<Lamp::RenderPipelineCompute> conversionPipeline = Lamp::RenderPipelineCompute::Create(Lamp::ShaderRegistry::Get("TextureConverter"));
		
		auto device = Lamp::GraphicsContext::GetDevice();
		VkCommandBuffer cmdBuffer = device->GetCommandBuffer(true);

		image->TransitionToLayout(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);

		conversionPipeline->Bind(cmdBuffer);
		conversionPipeline->SetImage(image, 0, 0, 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		conversionPipeline->SetTexture(inputTexture, 0, 1);

		struct PushConstants
		{
			int32_t redTarget;
			int32_t greenTarget;
			int32_t blueTarget;
			int32_t alphaTarget;

			glm::vec2 targetSize;
		} pushConstants;

		pushConstants.targetSize = glm::vec2(width, height);
		pushConstants.redTarget = 0;
		pushConstants.greenTarget = 1;
		pushConstants.blueTarget = 2;
		pushConstants.alphaTarget = 3;

		conversionPipeline->SetPushConstant(cmdBuffer, sizeof(PushConstants), &pushConstants);
		conversionPipeline->Dispatch(cmdBuffer, 0, width / 32, height / 32, 1);
		conversionPipeline->InsertBarrier(cmdBuffer, 0, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		device->FlushCommandBuffer(cmdBuffer);

		image->ReadMemoryToBuffer();

		Ref<Lamp::Texture2D> outputTexture = Lamp::Texture2D::Create(image);
		outputTexture->path = path.parent_path() / "output.dds";

		Lamp::AssetManager::Get().SaveAsset(outputTexture);
	}
}
