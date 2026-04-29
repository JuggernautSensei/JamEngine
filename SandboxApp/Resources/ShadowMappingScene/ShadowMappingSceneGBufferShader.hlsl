#include "ShadowMappingSceneCommon.hlsl"

struct VSInput
{
    float3 posL : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normalL : NORMAL;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normalW : NORMAL;
};

struct PSOutput
{
    float4 normalW : SV_TARGET0;
    float4 color : SV_TARGET1;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4 posW = mul(float4(input.posL, 1.0f), cb_world);
    output.posH = mul(posW, cb_viewProj);
    output.texcoord = input.texcoord;
    output.normalW = normalize(mul(float4(input.normalL, 0.0f), cb_world).xyz);
    return output;
}

PSOutput PSMain(PSInput input)
{
    PSOutput output;
    output.normalW = float4(input.normalW, 0.f);

    // color
    output.color;
    if (cb_bUseTexture)
    {
        output.color = t_baseColor.Sample(s_linearWrap, input.texcoord);
    }
    else
    {
        output.color = CvtRGBAToLinear(cb_rgba);
    }
    return output;
}