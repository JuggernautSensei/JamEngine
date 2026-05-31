#include "../Common/Common.hlsl"

#define TEXTURE_MAPPING_NONE        0
#define TEXTURE_MAPPING_ALBEDO      1 << 0
#define TEXTURE_MAPPING_AO          1 << 1
#define TEXTURE_MAPPING_NORMAL      1 << 2

static const float3 g_baseColor = float3(1.0f, 1.0f, 1.0f);

cbuffer CBEntity : register(b0)
{
    float4x4 cb_world;
    float4x4 cb_worldInvTrans;
    uint cb_flags;
    PADDING3;
};

cbuffer CBCamera : register(b1)
{
    float3 cb_eyePos;
    PADDING1;
    float4x4 cb_viewProj;
};

cbuffer CBLight : register(b2)
{
    float3 cb_lightDir;
    PADDING1;
    float3 cb_lightColor;
    PADDING1;
    float3 cb_ambientColor;
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

SamplerState s_linear : register(s0);
Texture2D t_albedo : register(t0);
Texture2D t_ao : register(t1);
Texture2D t_normal : register(t2);

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

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normalW);

    if (cb_flags & TEXTURE_MAPPING_NORMAL)
    {
        float3 T = normalize(input.tangentW);
        float3 B = normalize(cross(N, T));
        float3x3 TBN = float3x3(T, B, N);
        float3 normalMap = t_normal.Sample(s_linear, input.texcoord).xyz;
        N = normalize(mul(normalMap * 2.0f - 1.0f, TBN));
    }

    float3 L = normalize(-cb_lightDir);
    float3 E = normalize(cb_eyePos - input.posW);
    float3 H = normalize(L + E);
    
    float NdotL = max(dot(N, L), 0.0f);
    float NdotH = max(dot(N, H), 0.0f);
    
    float3 diffuse = cb_lightColor * g_baseColor * NdotL;
    float3 specular = cb_lightColor * pow(NdotH, 16);
    float3 ambient = cb_ambientColor * g_baseColor;
    
    if (cb_flags & TEXTURE_MAPPING_ALBEDO)
    {
        float3 albedo = t_albedo.Sample(s_linear, input.texcoord).xyz;
        diffuse *= albedo;
        ambient *= albedo;
    }
    
    if (cb_flags & TEXTURE_MAPPING_AO)
    {
        float ao = t_ao.Sample(s_linear, input.texcoord).x;
        diffuse *= ao;
        ambient *= ao;
    }
    
    return float4(diffuse + specular + ambient, 1.0f);
}