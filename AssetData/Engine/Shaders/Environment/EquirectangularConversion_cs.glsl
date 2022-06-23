#version 460

const float PI = 3.141592f;

layout(set = 0, binding = 0) uniform sampler2D u_equirectangularTexture;
layout(set = 0, binding = 1, rgba32f) restrict writeonly uniform imageCube o_cubeMap;

vec3 GetCubeMapTexCoord()
{
    vec2 st = gl_GlobalInvocationID.xy / vec2(imageSize(o_cubeMap));
    vec2 uv = 2.f * vec2(st.x, 1.f - st.y) - vec2(1.f);

    vec3 ret;
    if  (gl_GlobalInvocationID.z == 0) ret = vec3(1.0, uv.y, -uv.x);
    else if (gl_GlobalInvocationID.z == 1) ret = vec3(-1.0, uv.y, uv.x);
    else if (gl_GlobalInvocationID.z == 2) ret = vec3(uv.x, 1.0, -uv.y);
    else if (gl_GlobalInvocationID.z == 3) ret = vec3(uv.x, -1.0, uv.y);
    else if (gl_GlobalInvocationID.z == 4) ret = vec3(uv.x, uv.y, 1.0);
    else if (gl_GlobalInvocationID.z == 5) ret = vec3(-uv.x, uv.y, -1.0);
    
    return normalize(ret);
}

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main()
{
    vec3 cubeTC = GetCubeMapTexCoord();

    // Calculate sampling coords for equirectangular texture
	// https://en.wikipedia.org/wiki/Spherical_coordinate_system#Cartesian_coordinates

    float phi = atan(cubeTC.z, cubeTC.x);
    float theta = acos(cubeTC.y);

    vec2 uv = vec2(phi / (2.f * PI) + 0.5f, theta / PI);
    vec4 color = texture(u_equirectangularTexture, uv);

    imageStore(o_cubeMap, ivec3(gl_GlobalInvocationID), color);
}