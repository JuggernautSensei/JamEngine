#include "Common.hlsl"

struct VSInput
{
    float3 posL : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posH = mul(float4(input.posL, 1.0f), mul(cb_world, cb_viewProj));
    return output;
}

float4 PSMain(PSInput _) : SV_TARGET
{
    return float4(1.f, 1.f, 1.f, 1.f);
}