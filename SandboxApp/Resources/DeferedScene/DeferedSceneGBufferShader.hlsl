#include "../Common/Common.hlsl"

Texture2D t_albedo : register(t0);
Texture2D t_normal : register(t1);

SamplerState s_linearWrap : register(s0);

cbuffer CBDeferedSceneEntity : register(b0)
{
    float4x4 cb_world;
    float4x4 cb_worldInvTrans;
    uint cb_bHasTextures;
    PADDING3;
};

cbuffer CBDeferedSceneCamera : register(b1)
{
    float4x4 cb_viewProj;
    float4x4 cb_invViewProj;
    float3 cb_eyePos;
    PADDING1;
};

struct VSInput
{
    float3 pos : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normalW : NORMAL;
    float3 tangentW : TANGENT;
};

struct PSOutput
{
    float4 color : SV_TARGET0;
    float4 normal : SV_TARGET1;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4 posW = mul(float4(input.pos, 1.0f), cb_world);
    output.posH = mul(posW, cb_viewProj);
    output.posW = posW.xyz;
    output.texcoord = input.texcoord;
    output.normalW = normalize(mul(float4(input.normal, 0.0f), cb_worldInvTrans).xyz);
    output.tangentW = normalize(mul(float4(input.tangent, 0.0f), cb_worldInvTrans).xyz);
    return output;
}

PSOutput PSMain(PSInput input)
{
    float3 albedo = float3(1.0f, 1.0f, 1.0f);
    float3 N = normalize(input.normalW);
    if (cb_bHasTextures == 1)
    {
        albedo = t_albedo.Sample(s_linearWrap, input.texcoord).rgb;

        // normal
        float3 T = normalize(input.tangentW);
        float3 B = normalize(cross(N, T));
        float3x3 TBN = float3x3(T, B, N);
        N = t_normal.Sample(s_linearWrap, input.texcoord).rgb * 2.0f - 1.0f;
        N = normalize(mul(N, TBN));
    }

    // [-1, 1] -> [0, 1]
    N = N * 0.5f + 0.5f;

    PSOutput output;
    output.color = float4(albedo, 1.f);
    output.normal = float4(N, 1.f);
    return output;
}