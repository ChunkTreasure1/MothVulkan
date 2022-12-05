#version 460

#include "Common.h"
#include "Buffers.h"

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec3 a_bitangent;
layout(location = 4) in vec2 a_texCoords;

void main()
{
    const uint meshIndex = u_objectMap[gl_BaseInstance + gl_DrawID];
    const mat4 transform = u_objectBuffer[meshIndex].transform;
    const vec4 worldPosition = transform * vec4(a_position, 1.f);

    gl_Position = u_cameraData.viewProj * worldPosition;
}