#ifdef __HLSL__

#define ivec2 int2
#define ivec3 int3
#define ivec4 int4

#define uvec2 uint2
#define uvec3 uint3
#define uvec4 uint4

#define dvec2 double2
#define dvec3 double3
#define dvec4 double4

#define bvec2 bool2
#define bvec3 bool3
#define bvec4 bool4

#define vec2 float2
#define vec3 float3
#define vec4 float4

#define mat4 float4x4
#define mat3 float3x3
#define mat2 float2x3

#endif // __HLSL__

struct ObjectData
{
	mat4 transform;
	vec4 sphereBounds;
};

struct DirectionalLight
{
	vec4 direction;
	vec4 colorIntensity;
};

struct CameraData
{
	mat4 view;
	mat4 proj;
	mat4 viewProj;

	vec4 position;
};

struct PassData
{
	uint passIndex;
};

struct TargetData
{
	vec2 targetSize;
};

struct Material
{
	uint albedo;
	uint materialNormal;
};