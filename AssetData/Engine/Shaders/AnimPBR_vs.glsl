#include "Structures.glslh"

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec3 a_bitangent;
layout(location = 4) in vec2 a_texCoords;
layout(location = 5) in ivec4 a_boneIds;
layout(location = 6) in vec4 a_weights;

layout(location = 0) out OutData
{
    vec3 worldPosition;
    vec2 texCoords;
    mat3 TBN;

    // Debug
    flat uint drawId;
    vec3 localNormal;

} o_outData;


layout(std140, set = 0, binding = 0) uniform CameraBuffer
{
    CameraData u_cameraData;
};

layout(std430, set = 1, binding = 0) readonly buffer ObjectBuffer
{
    ObjectData u_objectBuffer[];
};

layout(std430, set = 1, binding = 1) readonly buffer ObjectMapBuffer
{
	uint u_objectMap[];
};

layout(std430, set = 2, binding = 0) readonly buffer AnimationDataBuffer
{
    mat4 bones[];
} u_AnimationData;

void main()
{
    const uint meshIndex = u_objectMap[gl_BaseInstance + gl_DrawID];
    const mat4 transform = u_objectBuffer[meshIndex].transform;

    mat4 skinnedMatrix = mat4(0);
    // loops through every boneid and weight in order x, y, z, w
    for(int i = 0; i < 4; ++i)
    {
        if(a_boneIds[i] == -1)
        {
            continue;
        }
        mat4 localMat = u_AnimationData.bones[a_boneIds[i]] * a_weights[i];
        skinnedMatrix += localMat;
    }
    const vec4 worldPos = vec4(a_position, 1.f) * skinnedMatrix;
    o_outData.worldPosition = worldPos.xyz;
    o_outData.texCoords = a_texCoords;
    o_outData.drawId = meshIndex;
    o_outData.localNormal = a_normal;

    // create the 3x3 TBN matrix
    vec3 newNormal = mat3(transform) * (a_normal * mat3(skinnedMatrix));
    vec3 newBiTangent = mat3(transform) * (a_bitangent * mat3(skinnedMatrix));
    vec3 newTangent = mat3(transform) * (a_tangent * mat3(skinnedMatrix));

    o_outData.TBN = mat3(newTangent, newBiTangent, newNormal);
}