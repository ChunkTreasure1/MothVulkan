#version 460

#include "Structures.glslh"

layout(location = 0) out vec4 o_positionMetallic;
layout(location = 1) out vec4 o_albedo;
layout(location = 2) out vec4 o_normalRoughness;

layout(set = 3, binding = 0) uniform sampler2D u_albedoTexture;
layout(set = 3, binding = 1) uniform sampler2D u_materialNormalTexture;

layout(location = 0) in InData
{
    vec3 worldPosition;
    vec2 texCoords;
    mat3 TBN;

    // Debug
    flat uint drawId;
    vec3 localNormal;

} v_input;

vec3 ReconstructNormal(vec3 normal)
{
    float normalZ = sqrt(clamp(dot(normal.xy, normal.xy), 0, 1));
    normal.z = normalZ;

    vec3 tangentNormal = normal * 2.f - 1.f;
    return(normalize(v_input.TBN * tangentNormal));
}

void main()
{
    const vec4 albedo = texture(u_albedoTexture, v_input.texCoords);
    const vec4 materialNormal = texture(u_materialNormalTexture, v_input.texCoords);
    const vec3 normal = ReconstructNormal(materialNormal.zyx);

    o_positionMetallic.xyz = v_input.worldPosition;
    o_positionMetallic.w = materialNormal.x;

    o_albedo = albedo;
    o_normalRoughness.xyz = normal;
    o_normalRoughness.w = materialNormal.w;
}