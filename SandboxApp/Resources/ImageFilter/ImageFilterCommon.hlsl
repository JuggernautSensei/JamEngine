#include "../Common/Common.hlsl"

struct VSInput
{
    float2 pos : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

cbuffer CBImageFilterParam : register(b0)
{
    float4 cb_params;
};

cbuffer CBImageFilterKernel : register(b1)
{
    float4 cb_weights[4];
    int cb_kernelLen;
    PADDING3;
};

float GetWeight(int _index)
{
    return cb_weights[_index / 4][_index % 4];
}

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posH = float4(input.pos, 0.f, 1.f);
    output.texcoord = CvtNdcToTex(input.pos);
    return output;
}

Texture2D t_input0 : register(t0);
Texture2D t_input1 : register(t1);