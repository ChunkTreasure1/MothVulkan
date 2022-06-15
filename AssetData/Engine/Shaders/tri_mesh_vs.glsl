#version 460

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec3 a_bitangent;
layout(location = 4) in vec2 a_texCoords;

layout(location = 0) out vec2 v_texCoords;
layout(location = 1) out flat uint v_drawId;

#include "common.glslh"

layout(std430, set = 1, binding = 0) readonly buffer ObjectBuffer
{
    ObjectData objects[];
} u_objectBuffer;

layout(std430, set = 1, binding = 1) readonly buffer ObjectMapBuffer
{
	uint objectMap[];
} u_objectMap;

void main()
{
    uint meshIndex = u_objectMap.objectMap[gl_BaseInstance];

    v_drawId = meshIndex;
    
    v_texCoords = a_texCoords;
    gl_Position = u_cameraBuffer.viewProj * u_objectBuffer.objects[meshIndex].transform * vec4(a_position, 1.f);
}