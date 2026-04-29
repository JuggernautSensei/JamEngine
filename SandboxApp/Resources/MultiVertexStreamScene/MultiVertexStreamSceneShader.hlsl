#include "../Common/Common.hlsl"   

cbuffer CBSimpleSceneEntity : register(b0)
{
    float4x4 cb_world;
};

cbuffer CBSimpleSceneCamera : register(b1)
{
    float3 cb_eyePos;
    PADDING1;
    float4x4 cb_viewProj;
};

struct VSInput
{
    float3 pos : POSITION; // 0
    uint4 color : COLOR; // 1
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4 posW = mul(float4(input.pos, 1.0f), cb_world);
    output.posH = mul(posW, cb_viewProj);
    output.color = input.color / 255.f;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}