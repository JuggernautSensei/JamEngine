#include "../Common.hlsl"

struct VSInput
{
    float2 pos : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posH = float4(input.pos, 0.f, 1.f);
    output.texcoord = CvtNdcToTex(input.pos);
    return output;
}

cbuffer CBImageFilter : register(b0)
{
    float4 params;
};

Texture2D t_input0;
Texture2D t_input1;
SamplerState s_linearWrap;
SamplerState s_linearClamp;