#version 460

layout(location = 0) in vec2 v_texCoords;
layout(location = 1) in flat uint v_drawId;

layout(location = 0) out vec4 o_color;

layout(set = 3, binding = 0) uniform sampler2D u_albedo;

void main()
{
    vec3 color = texture(u_albedo, v_texCoords).xyz;
    o_color = vec4(float(v_drawId) / 200);
    o_color.w = 1;
}