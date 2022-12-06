#version 460

//! #extension GL_KHR_vulkan_glsl : require

#include "Common.h" //! #include "../../Includes/Common.h"

layout(set = 2, binding = 6, rgba8) restrict writeonly uniform image2D o_output;

layout(set = 2, binding = 0) uniform sampler2D u_positionMetallic;
layout(set = 2, binding = 1) uniform sampler2D u_albedo;
layout(set = 2, binding = 2) uniform sampler2D u_normalRoughness;

layout(set = 2, binding = 3) uniform samplerCube u_irradianceTexture;
layout(set = 2, binding = 4) uniform samplerCube u_radianceTexture;
layout(set = 2, binding = 5) uniform sampler2D u_BRDFLut;

layout(std140, set = 1, binding = 0) uniform CameraBuffer
{
    CameraData u_cameraData;
};

layout(std140, set = 0, binding = 1) uniform DirectionalLightBuffer
{
    DirectionalLight u_directionalLight;
};

layout(std140, set = 1, binding = 1) uniform TargetBuffer
{
    TargetData u_targetData;
};

struct PBRParamters
{
    vec4 albedo;
    vec3 normal;
    float metallic;
    float roughness;

} m_pbrParameters;

const vec3 m_dielectricBase = vec3(0.04);
const float PI = 3.14159265359;
const float EPSILON = 0.0000001;

float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = NdotH * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;

    return a2 / max(denom, EPSILON);
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float ggx1 = NdotV / (NdotV * (1.0 - k) + k);
    float ggx2 = NdotL / (NdotL * (1.0 - k) + k);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(vec3 baseReflectivity, float HdotV)
{
    return baseReflectivity + (1.0 - baseReflectivity) * pow(1.0 - HdotV, 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 baseReflectivity, float roughness)
{
    return baseReflectivity + (max(vec3(1.f - roughness), baseReflectivity) - baseReflectivity) * pow(max(1.f - cosTheta, 0.f), 5.f);
}

vec3 CalculateDirectionalLight(DirectionalLight light, vec3 dirToCamera, vec3 baseReflectivity)
{
    const vec3 lightDir = normalize(light.direction.xyz);
    const vec3 H = normalize(dirToCamera + lightDir);

    // Cook-Torrance BRDF
    const float NdotV = max(dot(m_pbrParameters.normal, dirToCamera), EPSILON);
    const float NdotL = max(dot(m_pbrParameters.normal, lightDir), EPSILON);
    const float HdotV = max(dot(H, dirToCamera), 0.0);
    const float NdotH = max(dot(m_pbrParameters.normal, H), 0.0);

    const float distribution = DistributionGGX(NdotH, m_pbrParameters.roughness);
    const float geometric = GeometrySmith(NdotV, NdotL, m_pbrParameters.roughness);
    const vec3 fresnel = FresnelSchlick(baseReflectivity, HdotV);

    vec3 specular = distribution * geometric * fresnel;
    specular /= 4.0 * NdotV * NdotL;

    vec3 kD = vec3(1.0) - fresnel;
    kD *= 1.0 - m_pbrParameters.metallic;

    const vec3 result = (kD * m_pbrParameters.albedo.xyz / PI + specular) * NdotL * light.colorIntensity.w * light.colorIntensity.xyz;
    return result;
}

vec3 CalculateAmbiance(vec3 dirToCamera, vec3 baseReflectivity)
{
    const float NdotV = max(0.f, dot(m_pbrParameters.normal, dirToCamera));
    const vec3 irradiance = textureLod(u_irradianceTexture, m_pbrParameters.normal, 0).rgb;

    const vec3 F = FresnelSchlick(baseReflectivity, NdotV);
    const vec3 kD = mix(vec3(1.f) - F, vec3(0.f), m_pbrParameters.metallic);
    const vec3 diffuseIBL = kD * m_pbrParameters.albedo.xyz * irradiance;
    
    const uint radianceTextureLevels = textureQueryLevels(u_radianceTexture);
    const vec3 R = 2.f * NdotV * m_pbrParameters.normal - dirToCamera;
    const vec3 specularIrradiance = textureLod(u_radianceTexture, R, m_pbrParameters.roughness * radianceTextureLevels).rgb;

    const vec2 brdf = textureLod(u_BRDFLut, vec2(NdotV, m_pbrParameters.roughness), 0.f).rg;
    const vec3 specularIBL = (baseReflectivity * brdf.x + brdf.y) * specularIrradiance;

    return (diffuseIBL + specularIBL);
}

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main()
{
    vec2 texCoords = vec2(gl_GlobalInvocationID.xy) / vec2(u_targetData.targetSize);

    ///// Read GBuffer /////
    const vec4 positionMetallic = texture(u_positionMetallic, texCoords);
    const vec4 albedo = texture(u_albedo, texCoords);
    const vec4 normalRoughness = texture(u_normalRoughness, texCoords);
    ////////////////////////

    ivec2 outputLocation = ivec2(gl_GlobalInvocationID.xy);

    if (albedo.w == 0.f)
    {
        return;
    }

    const vec3 worldPosition = positionMetallic.xyz;

    m_pbrParameters.albedo = albedo;
    m_pbrParameters.normal = normalRoughness.xyz;
    m_pbrParameters.metallic = positionMetallic.w;
    m_pbrParameters.roughness = normalRoughness.w;

    const vec3 dirToCamera = normalize(u_cameraData.position.xyz - worldPosition);
    const vec3 baseReflectivity = mix(m_dielectricBase, m_pbrParameters.albedo.xyz, m_pbrParameters.metallic);

    vec3 lightAccumulation = vec3(0.f);
    lightAccumulation += CalculateDirectionalLight(u_directionalLight, dirToCamera, baseReflectivity);
    lightAccumulation += CalculateAmbiance(dirToCamera, baseReflectivity);

    const float gamma = 2.2f;
    lightAccumulation = pow(lightAccumulation, vec3(1.f / gamma));
  
    imageStore(o_output, outputLocation, vec4(lightAccumulation, 1.f));
}