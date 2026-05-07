#include "../Common/Common.hlsl"

struct VSInput
{
    float2 pos : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

Texture2D t_colorScene : register(t3);
Texture2D t_lightScene : register(t5);

// full screen tri
// [-1, 1] [3, 1] [-1, -3]
PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posH = float4(input.pos, 0.0f, 1.0f);
    output.texcoord = float2(output.posH.x, -output.posH.y) * 0.5f + 0.5f;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 color = t_colorScene.Sample(s_linearWrap, input.texcoord).xyz;
    float3 light = t_lightScene.Sample(s_linearWrap, input.texcoord).xyz;
    light += 0.1f; // ambient
    return float4(color * light, 1.0f);
}