#include "ShaderBase.hlsl"

//float Texture2DPCF(float2 uv,
//                   float zNDC,
//                   float radius,
//                   Texture2D shadowMapTexture)
//{
//    float shadowFactor = 0.f;

//    [unroll]
//    for (int j = 0; j < PCF_FILTER_SAMPLING_COUNT; j++)
//    {
//        float2 offset = poissonDisk2D[j] * radius;
//        shadowFactor += shadowMapTexture.SampleCmpLevelZero(SamplerComparisonLinearWrap,
//                                                           uv + offset,
//                                                           zNDC).r;
//    }

//    return shadowFactor / PCF_FILTER_SAMPLING_COUNT;

//}

//float Texture2DArrayPCF(float2 uv,
//                        uint arrayIndex,
//                        float depth,
//                        float radius,
//                        Texture2DArray shadowMapTextureArray)
//{
    
//    float shadowFactor = 0.f;

//    [unroll]
//    for (int j = 0; j < PCF_FILTER_SAMPLING_COUNT; j++)
//    {
//        float2 offset = poissonDisk2D[j] * radius;
//        shadowFactor += shadowMapTextureArray.SampleCmpLevelZero(SamplerComparisonLinearWrap,
//                                                           float3(uv + offset, arrayIndex),
//                                                           depth).r;
//    }

//    return shadowFactor / PCF_FILTER_SAMPLING_COUNT;
//}

//float TextureCubePCF(float3 cb_light_posW,
//                     float3 positionW,
//                     float3 cb_lightDirection,
//                     float lightcb_light_fallOffEnd,
//                     float radius,
//                     TextureCube shadowMapTextureCube)
//{
//    float shadowFactor = 0.f;

//    float lenToLight = distance(positionW, cb_light_posW) / lightcb_light_fallOffEnd;

//    [unroll]
//    for (int j = 0; j < PCF_FILTER_SAMPLING_COUNT; j++)
//    {
//        float3 offset = poissonDisk3D[j] * radius;
//        shadowFactor += shadowMapTextureCube.SampleCmpLevelZero(SamplerComparisonLinearWrap,
//                                                        -cb_lightDirection + offset,
//                                                        lenToLight).r;
//    }

//    return shadowFactor / PCF_FILTER_SAMPLING_COUNT;
//}

//void ComputeLight(
//    float3 positionW,
//    LightSubdata light,
//    uint index,
//    out float3 toLight,
//    out float3 radiance,
//    out float shadowFactor)
//{
//    toLight = float3(0.f, 0.f, 0.f);
//    radiance = float3(0.f, 0.f, 0.f);

//    // Compute Light
//    toLight = light.cb_lightType == JAM_LIGHT_TYPE_DIRECTIONAL
//        ? -light.cb_lightDirection
//        : normalize(light.cb_lightPosition - positionW);

//    float dist = distance(light.cb_lightPosition, positionW);

//    float attenuation = light.cb_lightType == JAM_LIGHT_TYPE_DIRECTIONAL
//                      ? 1.f
//                      : ComputeAttenuation(dist, light.cb_lightFallOffStart, light.cb_lightFallOffEnd);

//    // https://learn.microsoft.com/en-us/windows/uwp/graphics-concepts/light-types
//    float spotAngleCos = dot(-toLight, normalize(light.cb_lightDirection));
//    float cosInnerCone = cos(light.cb_lightInnerConeAngle * 0.5f);
//    float cosOuterCone = cos(light.cb_lightOuterConeAngle * 0.5f);
//    float spotFactor = light.cb_lightType == JAM_LIGHT_TYPE_SPOT
//                     ? (spotAngleCos - cosOuterCone) / (cosInnerCone - cosOuterCone)
//                     : 1.0;

//    radiance = light.cb_lightRadiance * light.cb_lightStrength
//             * attenuation * spotFactor;

//    radiance = light.cb_lightType == LIGHT_TYPE_NONE
//             ? float3(0.f, 0.f, 0.f) : radiance;

//    // Compute Shadow
//    shadowFactor = 1.f;

//    if (light.cb_lightType == LIGHT_TYPE_POINT)
//    {
//     //   float lenToLight = distance(positionW, light.cb_cb_light_pos) / light.cb_cb_light_fallOffEnd;
//     //
//     //   [unroll]
//     //   for (int j = 0; j < 64; j++)
//     //   {
//     //       float3 offset = g_diskSamples643D[j];
//     //       offset.x *= dx;
//     //       offset.y *= dy;
//     //       offset.z *= dz;
//     //       percentLit += OmniShadowCubeTexture[index].SampleCmpLevelZero(SamplerComparisonLinearWrap,
//     //                                                  -toLight + offset,
//     //                                                  lenToLight).r;
//     //   }
   
//    }
//    else if (light.cb_lightType == JAM_LIGHT_TYPE_DIRECTIONAL)
//    {
//        // for debug
//        static const float3 cascadeColor[4] =
//        {
//            float3(1.0f, 0.0f, 0.0f),
//            float3(0.0f, 1.0f, 0.0f),
//            float3(0.0f, 0.0f, 1.0f),
//            float3(1.0f, 1.0f, 0.0f)
//        };

//        static const float cascadeRadius[4] = { 3.0, 2.0, 1.0, 0.5 }; // heuristic

//        // Cascade Index 결정
//        float zInView = mul(float4(positionW, 1.0), cb_view).z;

//        float4 cascadeChecks = float4(
//                zInView < cb_cascadeRange1,
//                zInView < cb_cascadeRange2,
//                zInView < cb_cascadeRange3,
//                1.0f);

//        int cascadeIndex = 4 - dot(cascadeChecks, float4(1, 1, 1, 1)); // 0 ~ 3
//        //radiance += cascadeColor[cascadeIndex]; // for debug

//        // Light 관점 Depth 결정
//        float4 lightScreen = mul(float4(positionW, 1.0), cb_cascadeShadowReceiverViewProj[cascadeIndex]);
//        lightScreen.xyz /= lightScreen.w;

//        float2 lightTexCoord;
//        lightTexCoord.x = 0.5 * lightScreen.x + 0.5;
//        lightTexCoord.y = -0.5 * lightScreen.y + 0.5;

//        // 최종 depth
//        float bias = lerp(0.00005f, 0.f, lightScreen.z); // todo
//        float depth = lightScreen.z + bias;

//        // 샘플링 크기 설정
//        uint width, height, elem, numberOfLevels;
//        CascadeShadowMapTextureArray.GetDimensions(0, width, height, elem, numberOfLevels);

//        shadowFactor = Texture2DArrayPCF(lightTexCoord, cascadeIndex, depth, cascadeRadius[cascadeIndex] / width, CascadeShadowMapTextureArray);
//    }
//    else
//    {
//      // float4 lightScreen = mul(float4(positionW, 1.0), basicShadowViewProj[index]);
//      // lightScreen.xyz /= lightScreen.w;
//      //
//      // float2 lightTexCoord; 
//      // lightTexCoord.x = 0.5 * lightScreen.x + 0.5;
//      // lightTexCoord.y = -0.5 * lightScreen.y + 0.5;
//      //
//      // float bias = lerp(0.01f, 0.f, lightScreen.z);
//      // float depth = lightScreen.z + bias;
//      //
//      // [unroll]
//      // for (int j = 0; j < 64; j++)
//      // {
//      //     float2 offset = g_diskSamples642D[j];
//      //     offset.x *= dx;
//      //     offset.y *= dy;
//      //     percentLit += ShadowMapTexture[index].SampleCmpLevelZero(SamplerComparisonLinearWrap,
//      //                                                lightTexCoord.xy + offset,
//      //                                                depth).r;
//      // }
   
//    }

//    shadowFactor = light.cb_shadowMappingEnable
//             ? shadowFactor
//             : 1.f;
//}

float4 PSMain(FULLSCREEN_PS_INPUT _input) : SV_TARGET
{
    float3 posNdc;
    posNdc.x = _input.texCoord0.x * 2.f - 1.f;
    posNdc.y = _input.texCoord0.y * -2.f + 1.f;
    posNdc.z = t_depth.Sample(s_pointClamp, _input.texCoord0).r;

    float4 unprojedPos = mul(float4(posNdc, 1.f), cb_viewProjInv);
    float3 posW = unprojedPos.xyz / unprojedPos.w;

    float4 normalRoughness = t_gBufferNormalRoughness.Sample(s_linearWrap, _input.texCoord0);
    float3 normal = normalize(normalRoughness.xyz);
    float roughness = normalRoughness.w;

    float4 emissiveMetallic = t_gBufferEmissiveMetallic.Sample(s_linearWrap, _input.texCoord0);
    float3 emissive = emissiveMetallic.rgb;
    float metallic = emissiveMetallic.w;

    float4 albedoAo = t_gBufferAlbedoAO.Sample(s_linearWrap, _input.texCoord0);
    float3 albedo = albedoAo.rgb;
    float ao = albedoAo.a;

    float3 toEye = normalize(cb_eyePos - posW);

    float3 r = normalize(reflect(-toEye, normal));
    float3 diffuseIBL = DiffuseIBL(albedo, normal, toEye, metallic) * cb_irradianceMapIntensity;
    float3 specularIBL = SpecularIBL(albedo, normal, toEye, r, metallic, roughness) * cb_specularMapIntensity;

    // 최종 차폐
    float ssao = (cb_envFlags & JAM_ENVIRONMENT_FLAGS_SSAO) ? t_ssao.Sample(s_linearWrap, _input.texCoord0).r : 1.f;
    float finalAo = ssao * ao;

    float nDotE = max(0.f, dot(normal, toEye));
    float specularAo = SpecularOcclusion(finalAo, roughness, nDotE);

    // 최종 색상
    diffuseIBL *= finalAo;
    specularIBL *= specularAo;

    float3 finalColor = diffuseIBL + specularIBL + emissive;
    return float4(finalColor, 1.f);
}