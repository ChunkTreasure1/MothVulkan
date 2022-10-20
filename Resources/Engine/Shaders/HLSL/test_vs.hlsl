#include "Common.h"

struct Input
{
    [[vk::location(0)]] float3 position : a_position;
    [[vk::location(1)]] float3 normal : a_normal;
    [[vk::location(2)]] float3 tangent : a_tangent;
    [[vk::location(3)]] float3 binormal : a_binormal;
    [[vk::location(4)]] float2 texCoords : a_texCoords;
};

struct Output
{
	float4 position : SV_POSITION;

	[[vk::location(0)]] float3 worldPosition : v_worldPos;
	[[vk::location(1)]] float2 texCoords : v_texCoords;
	[[vk::location(2)]] float3x3 TBN : v_TBN;

	[[vk::location(3)]] uint drawId : v_drawId;
	[[vk::location(4)]] float3 localNormal : v_localNormal;
};

[[vk::binding(0, 0)]] ConstantBuffer<CameraData> u_cameraData;
[[vk::binding(0, 4)]] StructuredBuffer<ObjectData> u_objectBuffer;
[[vk::binding(1, 4)]] StructuredBuffer<uint> u_objectMap;

Output main(Input input) : SV_Position
{
	const uint meshIndex = u_objectMap[]

	Output output;

    return output;
}