#include "ShadowMappingSceneCommon.hlsl"

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
    output.posH = mul(float4(input.posL, 1.f), cb_world);
    output.posH = mul(output.posH, cb_light[0]);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(0.f, 0.f, 0.f, 0.f);
}