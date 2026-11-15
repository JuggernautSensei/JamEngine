#include "../Common/Common.hlsl"

#define MAX_SKIN_JOINTS 256

cbuffer CBEntity : register(b0)
{
    float4x4 cb_world;
    uint cb_bHasAlbedo;
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

cbuffer CBSkin : register(b3)
{
    float4x4 cb_joints[MAX_SKIN_JOINTS];
};

struct VSInput
{
    float3 pos : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    uint4 blendIndex : BlendIndex;
    float4 blendWeight : BlendWeight;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normalW : NORMAL;
};

SamplerState s_linear : register(s0);
Texture2D t_albedo : register(t0);

PSInput VSMain(VSInput input)
{
    float4x4 skin = cb_joints[input.blendIndex.x] * input.blendWeight.x
                  + cb_joints[input.blendIndex.y] * input.blendWeight.y
                  + cb_joints[input.blendIndex.z] * input.blendWeight.z
                  + cb_joints[input.blendIndex.w] * input.blendWeight.w;

    float4 posL = mul(float4(input.pos, 1.0f), skin);
    float3 normalL = mul(float4(input.normal, 0.0f), skin).xyz;

    float4 posW = mul(posL, cb_world);

    PSInput output;
    output.posH = mul(posW, cb_viewProj);
    output.posW = posW.xyz;
    output.texcoord = input.texcoord;
    output.normalW = normalize(mul(float4(normalL, 0.0f), cb_world).xyz);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normalW);
    float3 L = normalize(-cb_lightDir);
    float3 E = normalize(cb_eyePos - input.posW);
    float3 H = normalize(L + E);

    float3 albedo = float3(0.8f, 0.8f, 0.8f);
    if (cb_bHasAlbedo != 0)
    {
        albedo = t_albedo.Sample(s_linear, input.texcoord).xyz;
    }

    float NdotL = max(dot(N, L), 0.0f);
    float NdotH = max(dot(N, H), 0.0f);

    float3 diffuse = cb_lightColor * albedo * NdotL;
    float3 specular = cb_lightColor * pow(NdotH, 32) * 0.2f;
    float3 ambient = cb_ambientColor * albedo;
    return float4(diffuse + specular + ambient, 1.0f);
}
