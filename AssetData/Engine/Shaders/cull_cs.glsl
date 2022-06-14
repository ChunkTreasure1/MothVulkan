#version 460

#include "common.glslh"

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

layout(std430, set = 0, binding = 3) writeonly buffer ObjectMapBuffer
{
	uint objectMap[];
} u_objectMap;

layout(push_constant) uniform constants
{
	DrawCullData u_cullData;
};

layout (local_size_x = 256) in;
void main()
{
	uint globalId = gl_GlobalInvocationID.x;

	if (globalId < u_cullData.drawCount)
	{	
		uint batchId = u_drawBuffer.draws[globalId].batchId;
		atomicAdd(u_countBuffer.counts[batchId], 1);
	
		u_objectMap.objectMap[globalId] = u_drawBuffer.draws[globalId].objectId;
	}
}