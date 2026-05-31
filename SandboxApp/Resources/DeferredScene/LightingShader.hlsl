#include "../Common/Common.hlsl"

cbuffer CBCamera : register(b1)
{
    float4x4 cb_viewProj;
    float4x4 cb_invViewProj;
    float3 cb_eyePos;
    PADDING1;
};

cbuffer CBLight : register(b2)
{
    float3 cb_lightPos;
    float cb_lightRadius;
    float3 cb_lightCol;
    PADDING1;
};

struct VSInput
{
    float2 pos : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

Texture2D t_depthScene : register(t2);
Texture2D t_colorScene : register(t3);
Texture2D t_normalScene : register(t4);

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posH = float4(input.pos, 0.0f, 1.0f);
    output.texcoord = CvtNdcToTex(input.pos);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    // normal
    float3 N = t_normalScene.Sample(s_linearWrap, input.texcoord).rgb * 2.f - 1.f;
    N = normalize(N);

    // world
    float depthNdc = t_depthScene.Sample(s_linearWrap, input.texcoord).r;
    float3 posW = CvtNdcToWorld(CvtTexToNdc(input.texcoord), depthNdc, cb_invViewProj);

    // light color
    float3 L = cb_lightPos - posW;
    float dist = length(L);
    float distAtt = CalcDistanceAttenuation(dist, 0.f, cb_lightRadius);

    L /= dist;
    float3 V = normalize(cb_eyePos - posW);
    float blinePhong = CalcBlinePhong(L, V, N, 16.f);

    float3 light = cb_lightCol * blinePhong * distAtt;
    return float4(light, 1.f);
}