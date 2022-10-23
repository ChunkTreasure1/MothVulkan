#version 460

#include "Common.glslh"

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

layout(std140, set = 1, binding = 0) uniform CameraBuffer
{
    CameraData u_cameraData;
};

layout(std140, set = 0, binding = 1) uniform DirectionalLightBuffer
{
    DirectionalLight u_directionalLight;
};

layout(set = 0, binding = 2) uniform samplerCube u_irradianceTexture;
layout(set = 0, binding = 3) uniform samplerCube u_radianceTexture;
layout(set = 0, binding = 4) uniform sampler2D u_BRDFLut;

layout(set = 3, binding = 0) uniform sampler2D u_albedoTexture;
layout(set = 3, binding = 1) uniform sampler2D u_materialNormalTexture;

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

vec3 FresnelSchlick(float HdotV, vec3 baseReflectivity)
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
    const vec3 fresnel = FresnelSchlick(HdotV, baseReflectivity);

    vec3 specular = distribution * geometric * fresnel;
    specular /= 4.0 * NdotV * NdotL;

    vec3 kD = vec3(1.0) - fresnel;
    kD *= 1.0 - m_pbrParameters.metallic;

    const vec3 result = (kD * m_pbrParameters.albedo.xyz / PI + specular) * NdotL * light.colorIntensity.w * light.colorIntensity.xyz;
    return result;
}

vec3 CalculateAmbiance(vec3 dirToCamera, vec3 baseReflectivity)
{    
       const vec3 normal = m_pbrParameters.normal;

       const vec3 fresnel = FresnelSchlickRoughness(max(dot(normal, dirToCamera), 0.f), baseReflectivity, m_pbrParameters.roughness);
       const vec3 reflectVec = reflect(-dirToCamera, normal);
    
       const int maxReflectionLOD = textureQueryLevels(u_radianceTexture);
       const vec3 radianceColor = textureLod(u_radianceTexture, reflectVec, m_pbrParameters.roughness * float(maxReflectionLOD)).rgb;

       const vec2 envBRDF = texture(u_BRDFLut, vec2(max(dot(normal, dirToCamera), 0.f), m_pbrParameters.roughness)).rg;
       const vec3 irradiance = texture(u_irradianceTexture, normal).rgb;

       vec3 diffuse = m_pbrParameters.albedo.xyz * irradiance;
       vec3 specular = radianceColor * (fresnel * envBRDF.x + envBRDF.y);

       vec3 kS = FresnelSchlickRoughness(max(dot(normal, dirToCamera), 0.f), baseReflectivity, m_pbrParameters.roughness);
       vec3 kD = 1.f - kS;
       kD *= 1.f - m_pbrParameters.metallic;

       vec3 ambiance = (kD * diffuse + specular);
       return ambiance;
}

vec3 ReconstructNormal(vec3 normal)
{
    float normalZ = sqrt(clamp(dot(normal.xy, normal.xy), 0, 1));
    normal.z = normalZ;

    vec3 tangentNormal = normal * 2.f - 1.f;
    return(normalize(v_input.TBN * tangentNormal));
}

void main()
{
    /////Textures/////
    const vec4 materialNormal = texture(u_materialNormalTexture, v_input.texCoords);

    m_pbrParameters.albedo = texture(u_albedoTexture, v_input.texCoords);
    m_pbrParameters.normal = ReconstructNormal(materialNormal.zyx);
    m_pbrParameters.metallic = materialNormal.x;
    m_pbrParameters.roughness = materialNormal.w;
    //////////////////

    const vec3 dirToCamera = normalize(u_cameraData.position.xyz - v_input.worldPosition);
    const vec3 baseReflectivity = mix(m_dielectricBase, m_pbrParameters.albedo.xyz, m_pbrParameters.metallic);

    vec3 lightAccumulation = vec3(0.0);

    lightAccumulation += CalculateDirectionalLight(u_directionalLight, dirToCamera, baseReflectivity);
    //lightAccumulation += CalculateAmbiance(dirToCamera, baseReflectivity);

    const float gamma = 2.2;
    o_color.xyz = pow(lightAccumulation, vec3(1.f / gamma));
    o_color.w = 1.f;
}