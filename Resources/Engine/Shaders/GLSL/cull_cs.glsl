#version 460

#include "Common.h"

struct DrawCullData
{
	mat4 view;
	float P00, P11, zNear, zFar;
	float frustum[4];
	float lodBase, lodStep;
	float pyramidWidth, pyramidHeight;
	
	uint drawCount;
	
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

struct DrawCommand
{	
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	int vertexOffset;
	uint firstInstance;

	uint objectId;
	uint batchId;
	uint padding;
};

layout(std140, set = 0, binding = 1) readonly buffer DrawBuffer
{
	DrawCommand draws[];
} u_drawBuffer;

layout(std430, set = 0, binding = 2) writeonly buffer CountBuffer
{
	uint counts[];
} u_countBuffer;

layout(std430, set = 1, binding = 4) writeonly buffer ObjectMapBuffer
{
	uint objectMap[];
} u_objectMap;

layout(std430, set = 0, binding = 4) readonly buffer ObjectBuffer
{
    ObjectData objects[];
} u_objectBuffer;

layout(push_constant) uniform constants
{
	DrawCullData u_cullData;
};

bool IsVisible(uint objectId)
{
	vec4 sphereBounds = u_objectBuffer.objects[objectId].sphereBounds;

	vec3 center = sphereBounds.xyz;
	float radius = sphereBounds.w;

	center = (u_cullData.view * vec4(center, 1.f)).xyz;

	bool visible = true;

	visible = visible && center.z * u_cullData.frustum[1] - abs(center.x) * u_cullData.frustum[0] > -radius;
	visible = visible && center.z * u_cullData.frustum[3] - abs(center.y) * u_cullData.frustum[2] > -radius;

	if (u_cullData.distCull != 0)
	{
		visible = visible && center.z + radius > u_cullData.zNear && center.z - radius < u_cullData.zFar;
	}

	visible = visible || u_cullData.cullingEnabled == 0;
	return visible;
}

layout (local_size_x = 256) in;
void main()
{
	const uint globalId = gl_GlobalInvocationID.x;

	if (globalId < u_cullData.drawCount)
	{	
		const uint objectId = u_drawBuffer.draws[globalId].objectId;
		const bool visible = IsVisible(objectId);

		if (visible)
		{
			const uint batchId = u_drawBuffer.draws[globalId].batchId;
			const uint drawIndex = atomicAdd(u_countBuffer.counts[batchId], 1);
			const uint baseIndex = u_drawBuffer.draws[globalId].firstInstance;

			u_objectMap.objectMap[baseIndex + drawIndex] = objectId;
		}
	}
}