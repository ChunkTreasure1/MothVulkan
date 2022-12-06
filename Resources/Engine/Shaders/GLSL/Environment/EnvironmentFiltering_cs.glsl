#version 460

#define PARAM_LEVEL 0;
#define PARAM_ROUGHNESS u_roughness

const float PI = 3.141592f;
const float TWO_PI = 2 * PI;
const float EPSILON =  0.00001;

const uint NUM_SAMPLES = 1024;
const float INV_NUM_SAMPLES = 1.f / float(NUM_SAMPLES);

const int NUM_MIP_LEVELS = 1;

layout(set = 0, binding = 0) uniform samplerCube u_inputTexture;
layout(set = 0, binding = 1, rgba32f) restrict writeonly uniform imageCube o_output;

layout(push_constant) uniform PushConstants
{
    float u_roughness;
};

// Compute Van der Corput radical inverse
// See: http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// Sample i-th point from Hammersley point set of NumSamples points total.
vec2 SampleHammersley(uint i)
{
	return vec2(i * INV_NUM_SAMPLES, RadicalInverse_VdC(i));
}

// Importance sample GGX normal distribution function for a fixed roughness value.
// This returns normalized half-vector between Li & Lo.
// For derivation see: http://blog.tobias-franke.eu/2014/03/30/notes_on_importance_sampling.html
vec3 SampleGGX(float u1, float u2, float roughness)
{
	float alpha = roughness * roughness;

	float cosTheta = sqrt((1.0 - u2) / (1.0 + (alpha*alpha - 1.0) * u2));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta); // Trig. identity
	float phi = TWO_PI * u1;

	// Convert to Cartesian upon return.
	return vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float NDFGGX(float cosLh, float roughness)
{
	float alpha   = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
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

// Compute orthonormal basis for converting from tanget/shading space to world space.
void ComputeBasisVectors(const vec3 N, out vec3 S, out vec3 T)
{
	// Branchless select non-degenerate T.
	T = cross(N, vec3(0.0, 1.0, 0.0));
	T = mix(cross(N, vec3(1.0, 0.0, 0.0)), T, step(EPSILON, dot(T, T)));

	T = normalize(T);
	S = normalize(cross(N, T));
}

// Convert point from tangent/shading space to world space.
vec3 TangentToWorld(const vec3 v, const vec3 N, const vec3 S, const vec3 T)
{
	return S * v.x + T * v.y + N * v.z;
}

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main()
{
    ivec2 outputSize = imageSize(o_output);
    if (gl_GlobalInvocationID.x >= outputSize.x || gl_GlobalInvocationID.y >= outputSize.y)
    {
        return;
    }

    vec2 inputSize = vec2(textureSize(u_inputTexture, 0));
    float wt = 4.f * PI / (6 * inputSize.x * inputSize.y);

    vec3 N = GetCubeMapTexCoord();
    vec3 Lo = N;

    vec3 S, T;
    ComputeBasisVectors(N, S, T);

    vec3 color = vec3(0);
    float weight;

    for (uint i = 0; i < NUM_SAMPLES; i++)
    {
        vec2 u = SampleHammersley(i);
        vec3 Lh = TangentToWorld(SampleGGX(u.x, u.y, PARAM_ROUGHNESS), N, S, T);
        vec3 Li = 2.f * dot(Lo, Lh) * Lh - Lo;

        float cosLi = dot(N, Li);
        if (cosLi > 0.f)
        {
            float cosLh = max(dot(N, Lh), 0.f);
            float pdf = NDFGGX(cosLh, PARAM_ROUGHNESS) * 0.25f;

            float ws = 1.f / (NUM_SAMPLES * pdf);
            float mipLevel = max(0.5f * log2(ws / wt) + 1.f, 0.f);

            color += textureLod(u_inputTexture, Li, mipLevel).rgb * cosLi;
            weight += cosLi;
        }
    }

    color /= weight;
    imageStore(o_output, ivec3(gl_GlobalInvocationID), vec4(color, 1.f));
}