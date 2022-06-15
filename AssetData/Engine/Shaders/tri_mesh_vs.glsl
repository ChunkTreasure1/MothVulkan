#version 460

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec3 a_bitangent;
layout(location = 4) in vec2 a_texCoords;

#include "common.glslh"


layout(location = 0) out OutData
{
    vec3 normal;
    vec2 texCoords;

    // Debug
    flat uint drawId;

} o_outData;

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
    uint meshIndex = u_objectMap.objectMap[gl_BaseInstance + gl_DrawID];

    o_outData.normal = a_normal;
    o_outData.texCoords = a_texCoords;
    o_outData.drawId = meshIndex;

    gl_Position = u_cameraBuffer.viewProj * u_objectBuffer.objects[meshIndex].transform * vec4(a_position, 1.f);
}