#pragma once

#include "Lamp/Core/Base.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace Lamp
{
	class Image2D;

	/////Renderer structs/////
	struct Skybox
	{
		Ref<Image2D> irradianceMap;
		Ref<Image2D> radianceMap;
	};

	/////Shader structs/////
	struct CameraData
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewProj;
	};

	struct DirectionalLightData
	{
		glm::vec4 direction;
		glm::vec4 colorIntensity;
	};

	struct ObjectData
	{
		glm::mat4 transform;
		glm::vec4 sphereBounds;
	};	

	struct ObjectMapData
	{
		uint32_t id;
		uint32_t batchId;
	};

	struct GPUIndirectObject
	{
		VkDrawIndexedIndirectCommand command;
		uint32_t objectId;
		uint32_t batchId;
		uint32_t padding;
	};

	struct CullData
	{
		glm::mat4 view;
		float P00, P11, zNear, zFar;
		float frustum[4];
		float lodBase, lodStep;
		float pyramidWidth, pyramidHeight;

		uint32_t drawCount;

		int cullingEnabled;
		int lodEnabled;
		int occlusionEnabled;
		int distCull;
		int AABBcheck;

		float aabbmin_x;
		float aabbmin_y;
		float aabbmin_z;
		float aabbmax_x;
		float aabbmax_y;
		float aabbmax_z;
	};
}