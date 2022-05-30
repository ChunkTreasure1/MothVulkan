#version 450

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec3 a_bitangent;
layout(location = 4) in vec2 a_texCoords;

layout(push_constant) uniform constants
{
    vec4 data;
    mat4 transform;
    
} u_pushConstants;

layout(set = 0, binding = 0) uniform CameraBuffer
{
    mat4 view;
    mat4 proj;
    mat4 viewProj;

} u_cameraBuffer;

void main()
{
    gl_Position = u_cameraBuffer.viewProj * u_pushConstants.transform * vec4(a_position, 1.f);
}