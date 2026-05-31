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
    float4 cb_fparams;
    int4 cb_iparams;
};

cbuffer CBImageFilterKernel : register(b1)
{
    float4 cb_weights[4];
    int cb_length;
    PADDING3;
};

float GetWeight(uint _index)
{
    return cb_weights[_index / 4u][_index % 4u];
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
Texture2DArray t_arrayInput0 : register(t2);
Texture2DArray t_arrayInput1 : register(t3);

#if defined(TEXTURE2DARRAY_INPUT0)
#define TEXTURE2D_INPUT0 t_arrayInput0
#define SAMPLE_TEXTURE2D_INPUT0(s, coord) t_arrayInput0.Sample(s, float3(coord, cb_iparams.x))
#else
#define TEXTURE2D_INPUT0 t_input0
#define SAMPLE_TEXTURE2D_INPUT0(s, coord) t_input0.Sample(s, coord)
#endif

#if defined(TEXTURE2DARRAY_INPUT1)
#define TEXTURE2D_INPUT1 t_arrayInput1
#define SAMPLE_TEXTURE2D_INPUT1(s, coord) t_arrayInput1.Sample(s, float3(coord, cb_iparams.y))
#else
#define TEXTURE2D_INPUT1 t_input1
#define SAMPLE_TEXTURE2D_INPUT1(s, coord) t_input1.Sample(s, coord)
#endif

void GetInput0Resolution(out uint w, out uint h)
{
#ifdef TEXTURE2DARRAY_INPUT0
    uint l;
    t_arrayInput0.GetDimensions(w, h, l);
#else
    t_input0.GetDimensions(w, h);
#endif
}