#version 460

layout(set = 0, binding = 0) uniform samplerCube u_radianceMap;
layout(set = 0, binding = 1, rgba32f) restrict writeonly uniform imageCube o_output;

const float PI = 3.141592;
const float TWO_PI = 2.f * PI;
const float EPSILON = 0.00001;

layout(push_constant) uniform PushConstants
{
    uint u_sampleCount;
};

float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 SampleHammersley(uint i, uint samples)
{
	float invSamples = 1.0 / float(samples);
	return vec2(i * invSamples, RadicalInverse_VdC(i));
}

vec3 SampleHemisphere(float u1, float u2)
{
	const float u1p = sqrt(max(0.0, 1.0 - u1 * u1));
	return vec3(cos(TWO_PI * u2) * u1p, sin(TWO_PI * u2) * u1p, u1);
}

vec3 GetCubeMapTexCoord()
{
    vec2 st = gl_GlobalInvocationID.xy / vec2(imageSize(o_output));
    vec2 uv = 2.0 * vec2(st.x, 1.0 - st.y) - vec2(1.0);

    vec3 ret;
    if (gl_GlobalInvocationID.z == 0)      ret = vec3(  1.0, uv.y, -uv.x);
    else if (gl_GlobalInvocationID.z == 1) ret = vec3( -1.0, uv.y,  uv.x);
    else if (gl_GlobalInvocationID.z == 2) ret = vec3( uv.x,  1.0, -uv.y);
    else if (gl_GlobalInvocationID.z == 3) ret = vec3( uv.x, -1.0,  uv.y);
    else if (gl_GlobalInvocationID.z == 4) ret = vec3( uv.x, uv.y,   1.0);
    else if (gl_GlobalInvocationID.z == 5) ret = vec3(-uv.x, uv.y,  -1.0);
    return normalize(ret);
}

void ComputeBasisVectors(const vec3 N, out vec3 S, out vec3 T)
{
	// Branchless select non-degenerate T.
	T = cross(N, vec3(0.0, 1.0, 0.0));
	T = mix(cross(N, vec3(1.0, 0.0, 0.0)), T, step(EPSILON, dot(T, T)));

	T = normalize(T);
	S = normalize(cross(N, T));
}

vec3 TangentToWorld(const vec3 v, const vec3 N, const vec3 S, const vec3 T)
{
	return S * v.x + T * v.y + N * v.z;
}

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main()
{
    vec3 N = GetCubeMapTexCoord();

    vec3 S, T;
    ComputeBasisVectors(N, S, T);

    uint sampleCount = 64 * u_sampleCount;

    vec3 irradiance = vec3(0);
    for (uint i = 0; i < sampleCount; i++)
    {
        vec2 u = SampleHammersley(i, sampleCount);
        vec3 Li = TangentToWorld(SampleHemisphere(u.x, u.y), N, S, T);
        float cosTheta = max(0.f, dot(Li, N));

        irradiance += 2.f * textureLod(u_radianceMap, Li, 0).rgb * cosTheta;
    }

    irradiance /= vec3(sampleCount);
    imageStore(o_output, ivec3(gl_GlobalInvocationID), vec4(irradiance, 1.f));
}