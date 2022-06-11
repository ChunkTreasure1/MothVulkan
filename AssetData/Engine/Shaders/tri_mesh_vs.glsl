#version 460

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec3 a_bitangent;
layout(location = 4) in vec2 a_texCoords;

layout(location = 0) out vec2 v_texCoords;

struct ObjectData
{
    mat4 transform;
};

layout(set = 0, binding = 0) uniform CameraBuffer
{
    mat4 view;
    mat4 proj;
    mat4 viewProj;

} u_cameraBuffer;

layout(set = 1, binding = 0) readonly buffer ObjectBuffer
{
    ObjectData objects[];
} u_objectBuffer;


layout(push_constant) uniform PushConstant
{
    uint meshIndex;
} u_pushConstant;

void main()
{
    v_texCoords = a_texCoords;
    gl_Position = u_cameraBuffer.viewProj * u_objectBuffer.objects[u_pushConstant.meshIndex + gl_InstanceIndex].transform * vec4(a_position, 1.f);
}