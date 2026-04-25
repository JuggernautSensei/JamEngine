#include "ShadowMappingSceneCommon.hlsl"

struct VSInput
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float3 normalW : NORMAL;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posW = mul(float4(input.posL, 1.0f), cb_world).xyz;
    output.posH = mul(float4(output.posW, 1.f), mul(cb_view, cb_proj));
    output.normalW = normalize(mul(float4(input.normalL, 0.0f), cb_invTransWorld).xyz);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normalW);
    N = N * 0.5f + 0.5f; // [-1, 1] -> [0, 1]
    return float4(N, 1.f);
}