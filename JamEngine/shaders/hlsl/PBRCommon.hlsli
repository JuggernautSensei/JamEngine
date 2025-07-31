#ifndef PBR_COMMON_HLSL_H
#define PBR_COMMON_HLSL_H

#include "ShaderCommon.hlsl"

static const float3 k_dielectric = 0.04f;

float3 SchlickFresnel(float3 _f0, float _NdotH)
{
    return _f0 + (1.0 - _f0) * pow(2.0, (-5.55473 * _NdotH - 6.98316) * _NdotH);
}

float3 DiffuseIBL(float3 albedo, float3 N, float3 E, float metallic)
{
    float3 F0 = lerp(k_dielectric, albedo, metallic);
    float3 F = SchlickFresnel(F0, max(0.0, dot(N, E)));
    float3 kd = lerp(1.0 - F, 0.0, metallic);
    float3 diffuse = diffuseCubemap.Sample(samplerLinearWrap, N).rgb;
    return kd * albedo * diffuse;
}

float3 SpecularIBL(float3 albedo, float3 N, float3 E, float3 R, float metallic, float roughness)
{
    float2 specularBRDF = brdfTexture.SampleLevel(samplerLinearClamp, float2(dot(N, E), 1.0 - roughness), 0.0f).rg;
    float3 specularIrradiance = specularCubemap.SampleLevel(samplerLinearWrap, R, roughness * 5.0f).rgb;
    float3 F0 = lerp(k_dielectric, albedo, metallic);
    return (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;
}

float3 AmbientLightingByIBL(float3 albedo, float3 N, float3 E, float ao, float metallic, float roughness)
{
    float3 R = normalize(reflect(-E, N));
    float3 diffuseIBL = DiffuseIBL(albedo, N, E, metallic);
    float3 specularIBL = SpecularIBL(albedo, N, E, R, metallic, roughness);
    return (diffuseIBL + specularIBL) * ao;
}

float NdfGGX(float NdotH, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;
    float denom = (NdotH * NdotH) * (alphaSq - 1.0) + 1.0;
    denom = max(denom, 1e-5);
    return alphaSq / (3.141592 * denom * denom);
}

float SchlickG1(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0 - k) + k);
}

float SchlickGGX(float NdotI, float NdotO, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return SchlickG1(NdotI, k) * SchlickG1(NdotO, k);
}

float Random(float3 seed, int i)
{
    float4 seed4 = float4(seed, i);
    float dot_product = dot(seed4, float4(12.9898, 78.233, 45.164, 94.673));
    return frac(sin(dot_product) * 43758.5453);
}

float3 DirectBRDF(float3 lightDirection, float3 toEye, float3 normal, float3 albedo, float metallic, float roughness)
{
    float3 halfway = normalize(toEye + lightDirection);
        
    float NdotI = max(0.0, dot(normal, lightDirection));
    float NdotH = max(0.0, dot(normal, halfway));
    float NdotO = max(0.0, dot(normal, toEye));
        
    float3 F0 = lerp(k_dielectric, albedo, metallic);
    float3 F = SchlickFresnel(F0, max(0.0, dot(halfway, toEye)));
    float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metallic);
    float3 diffuseBRDF = kd * albedo;

    float D = NdfGGX(NdotH, roughness);
    float3 G = SchlickGGX(NdotI, NdotO, roughness);
    float3 specularBRDF = (F * D * G) / max(1e-5, 4.0 * NdotI * NdotO);
    
    return (diffuseBRDF + specularBRDF) * NdotI;
}


float3 ComputeLightMap(uint bUseLightMap, float lightMapStrength, float2 texCoord)
{
    uint bEnable = step(0.5f, bUseLightMap);
    float3 lightTexColor = lightmapTexture.Sample(samplerLinearWrap, texCoord).rgb * lightMapStrength;
    float3 output = lerp(float3(1.f, 1.f, 1.f), lightTexColor, bEnable);
    return output;
}

struct PBR_PS_INPUT
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float2 texCoord : TEXCOORD;
    float2 texCoord2 : TEXCOORD2;
    float3 normalW : NORMAL;
    float3 tangentW : TANGENT;
    float3 bitangentW : BITANGENT;
};

#endif // PBR_COMMON_HLSL_H
