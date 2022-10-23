#version 460

#include "Structures.glslh"

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec3 a_bitangent;
layout(location = 4) in vec2 a_texCoords;

layout(location = 0) out OutData
{
    vec3 worldPosition;
    vec2 texCoords;
    mat3 TBN;

    // Debug
    flat uint drawId;
    vec3 localNormal;

} o_outData;

layout(std140, set = 1, binding = 0) uniform CameraBuffer
{
    CameraData u_cameraData;
};

layout(std140, set = 1, binding = 2) uniform PassBuffer
{
    PassData u_passData;
};

layout(std430, set = 4, binding = 0) readonly buffer ObjectBuffer
{
    ObjectData u_objectBuffer[];
};

layout(std430, set = 4, binding = 1) readonly buffer ObjectMapBuffer
{
	uint u_objectMap[];
};

void main()
{
    const uint meshIndex = u_objectMap[gl_BaseInstance + gl_DrawID];
    const mat4 transform = u_objectBuffer[meshIndex].transform;
    const vec4 worldPosition = transform * vec4(a_position, 1.f);

    o_outData.worldPosition = worldPosition.xyz;
    o_outData.texCoords = a_texCoords;

    o_outData.localNormal = a_normal;
    o_outData.drawId = meshIndex;

    const mat3 worldNormalRotation = mat3(transform);
    const vec3 T = normalize(worldNormalRotation * a_tangent);
    const vec3 B = normalize(worldNormalRotation * a_bitangent);
    const vec3 N = normalize(worldNormalRotation * a_normal);

    o_outData.TBN = mat3(T, B, N);

    gl_Position = u_cameraData.viewProj * worldPosition;
}