#ifndef SHADER_COMMON_HLSL_H
#define SHADER_COMMON_HLSL_H

#include "../../EmbeddedShaderGlue.h"

//////////////////////////////////
///          CONSTANT          ///
//////////////////////////////////

static const float3 k_dielectric = float3(0.04f, 0.04f, 0.04f);
static const float k_epsilon = 1e-5f;
static const float k_alphaCutoutThreshold = 0.01f;

/////////////////////////////////
///          STRUCTS          ///
/////////////////////////////////

struct PBR_PS_INPUT_STATIC_MESH
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float3 normalW : NORMAL;
    float3 tangentW : TANGENT;
    float2 texCoord0 : TEXCOORD0;
    float2 texCoord1 : TEXCOORD1;
};

struct FULLSCREEN_PS_INPUT
{
    float4 posH : SV_POSITION;
    float2 texCoord0 : TEXCOORD0;
};

//////////////////////////////////////////////
///          ATTENUATION FUNCTIONS         ///
//////////////////////////////////////////////

float CombindAttenuation(float _distance, float _k1, float _k2, float _k3)
{
    return 1.f / (_k1 + _k2 * _distance + _k3 * _distance * _distance);
}

float SmoothAttenuation(float _distance, float _fallOffStart, float _fallOffEnd)
{
    float value = 1.f - saturate((_distance - _fallOffStart) / (_fallOffEnd - _fallOffStart));
    return smoothstep(0.f, 1.f, value);
}

////////////////////////////////////////
///          MATH FUNCTIONS          ///
////////////////////////////////////////

// Hermite interpolation
// _p0 : start point
// _m0 : start tangent
// _p1 : end point
// _m1 : end tangent
// _t  : interpolation factor [0, 1]
float Hermite(float _p0, float _m0, float _p1, float _m1, float _t)
{
    float t2 = _t * _t;
    float t3 = t2 * _t;

    float h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
    float h10 = t3 - 2.0 * t2 + _t;
    float h01 = -2.0 * t3 + 3.0 * t2;
    float h11 = t3 - t2;

    return h00 * _p0 + h10 * _m0 + h01 * _p1 + h11 * _m1;
}

float3 SchlickFresnel(float3 _f0, float _nDoth)
{
    return _f0 + (1.0 - _f0) * pow(2.0, (-5.55473 * _nDoth - 6.98316) * _nDoth);
}

float3 SchlickFresnelRoughness(float3 _f0, float _nDote, float _roughness)
{
    float3 invRoughness = 1.f - _roughness;
    float cosi = clamp(_nDote, 0.0, 1.0);
    return _f0 + (max(invRoughness, _f0) - _f0) * pow(1.f - cosi, 5.0);
}

float Gaussian(float x, float sigma)
{
    return exp(-(x * x) / (2.0 * sigma * sigma));
}

/////////////////////////////
///          PBR          ///
/////////////////////////////

float3 DiffuseIBL(float3 _albedo, float3 _normal, float3 _fresnel, float _metallic)
{
    float3 kD = 1.0 - _fresnel;
    kD *= 1.0 - _metallic;
    float3 irradiance = t_irradiance.Sample(s_linearWrap, _normal).rgb * cb_irradianceMapIntensity;
    return kD * _albedo * irradiance;
}

float3 SpecularIBL(float3 _reflect, float _nDote, float _roughness, float3 _fresnel)
{
    float lod = _roughness * cb_specularMapNumMips;
    float3 specularIrradiance = t_specular.SampleLevel(s_linearWrap, _reflect, lod).rgb * cb_specularMapIntensity;
    float2 specularBRDF = t_brdf.Sample(s_linearClamp, float2(_nDote, _roughness)).rg;
    return specularIrradiance * (_fresnel * specularBRDF.x + specularBRDF.y);
}

float NdfGGX(float _nDoth, float _roughness)
{
    float alpha = _roughness * _roughness;
    float alphaSq = alpha * alpha;
    float denom = (_nDoth * _nDoth) * (alphaSq - 1.0) + 1.0;
    denom = max(denom, 1e-5);
    return alphaSq / (3.141592 * denom * denom);
}

float SchlickG1(float _nDotv, float _k)
{
    return _nDotv / (_nDotv * (1.0 - _k) + _k);
}

float SchlickGGX(float _nDotl, float _nDote, float _roughness)
{
    float r = _roughness + 1.0;
    float k = (r * r) / 8.0;
    return SchlickG1(_nDotl, k) * SchlickG1(_nDote, k);
}

float Random(float3 _seed, int _i)
{
    float4 seed4 = float4(_seed, _i);
    float dot_product = dot(seed4, float4(12.9898, 78.233, 45.164, 94.673));
    return frac(sin(dot_product) * 43758.5453);
}

float3 DirectBRDF(float3 _toLight, float3 _toEye, float3 _normal, float3 _albedo, float _metallic, float _roughness)
{
    float3 halfway = normalize(_toEye + _toLight);
        
    float nDotl = max(0.0, dot(_normal, _toLight));
    float nDoth = max(0.0, dot(_normal, halfway));
    float nDote = max(0.0, dot(_normal, _toEye));
        
    float3 F0 = lerp(k_dielectric, _albedo, _metallic);
    float hDote = max(0.0, dot(halfway, _toEye));
    float3 F = SchlickFresnel(F0, hDote);
    float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), _metallic);
    float3 diffuseBRDF = kd * _albedo;

    float D = NdfGGX(nDoth, _roughness);
    float G = SchlickGGX(nDotl, nDote, _roughness);
    float3 specularBRDF = (F * D * G) / max(1e-5, 4.0 * nDotl * nDote);
    
    return (diffuseBRDF + specularBRDF) * nDotl;
}

///////////////////////////////////
///          UTILITIES          ///
///////////////////////////////////

float3 CalcNormal(float3 _tangent, float3 _normal, float2 _texCoord)
{
    float3 normal = normalize(_normal);
    if (cb_materialFlags & JAM_MATERIAL_FLAGS_NORMAL)
    {
        float3 texNormal = t_normal.Sample(s_linearWrap, _texCoord).rgb;
        texNormal = normalize(2.f * texNormal - 1.f); // [0, 1] -> [-1, 1]
        texNormal.y = (cb_materialFlags & JAM_MATERIAL_FLAGS_FLIP_Y_NORMAL) ? -texNormal.y : texNormal.y;

        float3 tangent = normalize(_tangent);
        float3 bitangent = normalize(cross(normal, tangent));
        float3x3 TBN = float3x3(tangent, bitangent, normal);
        normal = normalize(mul(texNormal, TBN));
    }
    return normal;
}

float NDCDepthToViewDepth(float _depthNDC, float4x4 _projMat)
{
    float A = _projMat._33;
    float B = _projMat._43;
    return B / (_depthNDC - A);
}

float NDCDepthToViewDepth(float _depthNDC, float _nearPlane, float _farPlane)
{
    return (_nearPlane * _farPlane) / (_farPlane - _depthNDC * (_farPlane - _nearPlane));
}

#endif // SHADER_COMMON_HLSL_H
