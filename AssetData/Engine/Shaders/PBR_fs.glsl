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

struct PBRParamters
{
    vec4 albedo;
    vec3 normal;
    float metallic;
    float roughness;

} m_pbrParameters;

vec3 ReconstructNormal(vec3 normal)
{
    float normalZ = sqrt(1.f - clamp(dot(normal.xy, normal.xy), 0, 1));
    normal.z = normalZ;

    vec3 tangentNormal = normal * 2.f - 1.f;
    return(normalize(v_input.TBN * tangentNormal));
}

void main()
{
    m_pbrParameters.albedo = texture(u_albedoTexture, v_input.texCoords);
    m_pbrParamerters.normal = ReconstructNormal(texture(u_normalTexture.wyz, v_input.texCoords));
    const vec4 material = texture(u_materialTexture, v_input.texCoords);

    m_pbrParamerters.metallic = material.x;
    m_pbrParamerters.roughness = material.y;

    o_color.xyz = color * max(dot(v_input.TBN[2], u_directionalLight.direction.xyz), 0.01) + vec3(0.1, 0.1, 0.1) * color;
    o_color.w = 1;
}