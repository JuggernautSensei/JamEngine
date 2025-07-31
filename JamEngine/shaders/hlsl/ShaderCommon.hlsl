#ifndef SHADER_COMMON_HLSL_H
#define SHADER_COMMON_HLSL_H

#include "../../ShaderBridge.h"

// far: close to 0, near: close to 1
float ComputeAttenuation(float _distance, float _fallOffStart, float _fallOffend)
{
    float value = 1.f - saturate((_distance - _fallOffStart) / (_fallOffend - _fallOffStart));
    value = smoothstep(0.f, 1.f, value);
    return value;
}

float Hermite(float p0, float m0, float p1, float m1, float t)
{
    float t2 = t * t;
    float t3 = t2 * t;

    float h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
    float h10 = t3 - 2.0 * t2 + t;
    float h01 = -2.0 * t3 + 3.0 * t2;
    float h11 = t3 - t2;

    return h00 * p0 + h10 * m0 + h01 * p1 + h11 * m1;
}

float3 GetNormal(float3 tangent, float3 bitangent, float3 normal, float2 texCoord)
{
    float3 output = normal;
    if (cb_materialTextureBindFlags & (JAM_MATERIAL_TEXTURE_BIND_FLAGS_NORMAL_GL | JAM_MATERIAL_TEXTURE_BIND_FLAGS_NORMAL_DX))
    {
        float3 normalTex = normalTexture.Sample(samplerLinearWrap, texCoord).rgb;
        normalTex = normalize(2.f * normalTex - 1.f);
        normalTex.y = (cb_materialTextureBindFlags & JAM_MATERIAL_TEXTURE_BIND_FLAGS_NORMAL_GL) ? -normalTex.y : normalTex.y;

        float3x3 TBN = float3x3(tangent, bitangent, normal);
        output = normalize(mul(normalTex, TBN));
    }
  
    return output;
}

float3 UnProjectionNDCPos(float3 _posInNDC, JAM_MATRIX _invMatrix)
{
    float4 unprojedPos = mul(float4(_posInNDC, 1.0f), _invMatrix);
    return unprojedPos.xyz / unprojedPos.w;
}

float ConvertDepthInNDCToDepthInView(float _depthNDC, JAM_MATRIX _projectionMatrix)
{
    float A = _projectionMatrix._33;
    float B = _projectionMatrix._43;
    return B / (_depthNDC - A);
}

struct SCREENSPACE_EFFECT_PS_INPUT
{
    float4 positionH : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

#endif // SHADER_COMMON_HLSL_H
