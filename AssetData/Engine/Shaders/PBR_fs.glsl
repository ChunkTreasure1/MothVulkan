#version 460

#include "Structures.glslh"

layout(location = 0) out vec4 o_color;

layout(location = 0) in InData
{
    vec3 worldPosition;
    vec2 texCoords;
    mat3 TBN;

    // Debug
    flat uint drawId;
    vec3 localNormal;

} v_input;

layout(std140, set = 0, binding = 1) uniform DirectionalLightBuffer
{
    DirectionalLight u_directionalLight;
};

layout(set = 3, binding = 0) uniform sampler2D u_albedoTexture;
layout(set = 3, binding = 1) uniform sampler2D u_normalTexture;
layout(set = 3, binding = 2) uniform sampler2D u_materialTexture;

void main()
{
    const vec3 color = vec3(1, 0, 0);

    o_color.xyz = color * max(dot(v_input.TBN[2], u_directionalLight.direction.xyz), 0.01) + vec3(0.1, 0.1, 0.1) * color;
    o_color.w = 1;
}