#version 460

#include "Common.h" //! #include "../../Includes/Common.h"
#include "Buffers.h" //! #include "../../Includes/Buffers.h"

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec3 a_bitangent;
layout(location = 4) in vec2 a_texCoords;

layout(push_constant) uniform constants
{
    mat4 transform;
    uint u_materialId;
};

layout(location = 0) out outData
{
    vec2 texCoords;

} o_outData;

void main()
{
    o_outData.texCoords = a_texCoords;

    const vec4 worldPosition = transform * vec4(a_position, 1.f);
    gl_Position = u_cameraData.viewProj * worldPosition;
}