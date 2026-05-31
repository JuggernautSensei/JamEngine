#include "../Common/Common.hlsl"

cbuffer CBEntity : register(b0)
{
    float4x4 cb_world;
    float4x4 cb_invTransWorld;
    float3 cb_baseColor;
    PADDING1;
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
    float3 normal : NORMAL;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float3 normalW : NORMAL;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4 posW = mul(float4(input.pos, 1.0f), cb_world);
    output.posH = mul(posW, cb_viewProj);
    output.posW = posW.xyz;
    output.normalW = normalize(mul(float4(input.normal, 0.0f), cb_invTransWorld).xyz);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normalW);
    float3 L = normalize(-cb_lightDir);
    float3 E = normalize(cb_eyePos - input.posW);
    float3 H = normalize(L + E); // half way vector for blinn-phong specular
    float NdotL = max(dot(N, L), 0.0f);
    float NdotH = max(dot(N, H), 0.0f);
    float3 diffuse = cb_lightColor * cb_baseColor * NdotL;
    float3 specular = cb_lightColor * pow(NdotH, 16.f); // blinn-phong specular with shininess of 16
    float3 ambient = cb_ambientColor * cb_baseColor;
    return float4(diffuse + specular + ambient, 1.0f);
}