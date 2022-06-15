#version 460

layout(location = 0) out vec4 o_color;

layout(location = 0) in InData
{
    vec3 normal;
    vec2 texCoords;

    // Debug
    flat uint drawId;

} v_input;

layout(set = 3, binding = 0) uniform sampler2D u_albedo;

void main()
{
    const vec3 color = vec3(1, 0, 1);

    o_color.xyz = color * max(dot(v_input.normal, vec3(0, 1, 0)), 0.01) + vec3(0.1, 0.1, 0.1) * color;
    o_color.w = 1;
}