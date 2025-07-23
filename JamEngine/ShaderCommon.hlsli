#ifndef SHADER_COMMON_HLSL_H
#define SHADER_COMMON_HLSL_H

#include "ShaderBridge.h"

//// far: close to 0, near: close to 1
//float ComputeAttenuation(float _distance, float _cb_light_fallOffStart, float _cb_light_fallOffEnd)
//{
//    float att = 1.f - saturate((_distance - _cb_light_fallOffStart) / (_cb_light_fallOffEnd - _cb_light_fallOffStart));
//    att = smoothstep(0.f, 1.f, att);
//    return att;
//}

//float Hermite(float p0, float m0, float p1, float m1, float t)
//{
//    float t2 = t * t;
//    float t3 = t2 * t;

//    float h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
//    float h10 = t3 - 2.0 * t2 + t;
//    float h01 = -2.0 * t3 + 3.0 * t2;
//    float h11 = t3 - t2;

//    return h00 * p0 + h10 * m0 + h01 * p1 + h11 * m1;
//}


//float3 GetNormal(float3 tangent,
//                 float3 bitangent,
//                 float3 normal,
//                 float2 texCoord,
//                 Texture2D normalMap)
//{
//    float3 output = normal;

//    if (cb_material_textureFlags & (MATERIAL_TEXTURE_FLAGS_NORMAL_GL | MATERIAL_TEXTURE_FLAGS_NORMAL_DX))
//    {
//        float3 normalTex = normalMap.Sample(SamplerLinearWrap, texCoord).rgb;
//        normalTex = normalize(2.0 * normalTex - 1.0);

//        normalTex.y = (cb_material_textureFlags & MATERIAL_TEXTURE_FLAGS_NORMAL_GL) ? -normalTex.y : normalTex.y;

//        float3x3 TBN = float3x3(tangent, bitangent, normal);
//        output = normalize(mul(normalTex, TBN));
//    }
    
//    return output;
//}

//float3 UnProjectionNDCPos(float3 posNDC, MATRIX invMatrix)
//{
//    float4 invProjPos = mul(float4(posNDC, 1.0f), invMatrix);
//    return invProjPos.xyz / invProjPos.w;
//}

//float NDCDepthToViewDepth(float depthNDC, MATRIX projectionMatrix)
//{
//    float A = projectionMatrix._33;
//    float B = projectionMatrix._43;
//    return B / (depthNDC - A);
//}

#endif // SHADER_COMMON_HLSL_H