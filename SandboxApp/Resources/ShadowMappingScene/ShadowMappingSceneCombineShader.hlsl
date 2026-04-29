#include "ShadowMappingSceneCommon.hlsl"

struct VSInput
{
    float2 posL : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posH = float4(input.posL, 0.f, 1.f);
    output.texcoord = CvtNdcToTex(input.posL);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 color = t_sceneColor.Sample(s_linearClamp, input.texcoord).rgb;
    float3 light = t_sceneLight.Sample(s_linearClamp, input.texcoord).rgb;
    light += kAmbientColor;
    return float4(color * light, 1.f);
}