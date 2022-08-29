#include "Common.hlslh"

struct VSInput
{
    [[vk::location(0)]] float3 position : a_position;
    [[vk::location(1)]] float3 normal : a_normal;
    [[vk::location(2)]] float3 tangent : a_tangent;
    [[vk::location(3)]] float3 binormal : a_binormal;
    [[vk::location(4)]] float2 texCoords : a_texCoords;
};

struct VSOutput
{
    float4 position : SV_POSITION;

    [[vk::location(0)]] float3 worldPosition : v_worldPos;
    [[vk::location(1)]] float2 texCoords : v_texCoords;
    [[vk::location(2)]] float3x3 TBN : v_TBN;

    [[vk::location(3)]] uint drawId : v_drawId;
    [[vk::location(4)]] float3 localNormal : v_localNormal;
};

ConstantBuffer<CameraData> u_cameraData : register(b0, space0);
StructuredBuffer<ObjectData> u_objectBuffer : register(b0, space4);
StructuredBuffer<uint> u_objectMat : register(b1, space4);

VSOutput main(VSInput input)
{
    VSOutput output;
    
    return output;
}