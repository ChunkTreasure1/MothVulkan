#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace Lamp
{
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