#include "ShadowMappingSceneCommon.hlsl"

struct VSInput
{
    float3 posL : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float4 posClip : TEXCOORD0; // for gbuffer sampling
};

float CalcHardShadow(float2 _posLightTex, float _currentDepth, float _bias)
{
    float closestDepth = t_shadowMap.Sample(s_linearClamp, _posLightTex).r;
    return step(_currentDepth - _bias, closestDepth);
}

float CalcPCF3(float2 _posLightTex, float _currentDepth, float _bias)
{
    uint w, h;
    t_shadowMap.GetDimensions(w, h);
    float2 texel = float2(1.f / w, 1.f / h);
    float shadow = 0.f;
    [unroll]
    for (int i = -1; i <= 1; ++i)
    {
        [unroll]
        for (int j = -1; j <= 1; ++j)
        {
            float2 texcoord = _posLightTex + float2(i, j) * texel;
            shadow += CalcHardShadow(texcoord, _currentDepth, _bias);
        }
    }
    return shadow / 9.f;
}

float CalcPCF5(float2 _posLightTex, float currentDepth, float bias)
{
    uint w, h;
    t_shadowMap.GetDimensions(w, h);
    float2 texel = float2(1.f / w, 1.f / h);
    float shadow = 0.f;
    [unroll]
    for (int i = -2; i <= 2; ++i)
    {
        [unroll]
        for (int j = -2; j <= 2; ++j)
        {
            float2 texcoord = _posLightTex + float2(i, j) * texel;
            shadow += CalcHardShadow(texcoord, currentDepth, bias);
        }
    }
    return shadow / 25.f;
}

float CalcPCF7(float2 _posLightTex, float _currentDepth, float _bias)
{
    uint w, h;
    t_shadowMap.GetDimensions(w, h);
    float2 texel = float2(1.f / w, 1.f / h);
    float shadow = 0.f;
    [unroll]
    for (int i = -3; i <= 3; ++i)
    {
        [unroll]
        for (int j = -3; j <= 3; ++j)
        {
            float2 texcoord = _posLightTex + float2(i, j) * texel;
            shadow += CalcHardShadow(texcoord, _currentDepth, _bias);
        }
    }
    return shadow / 49.f;
}

float CalcESM(float2 _posLightTex, float currentDepth)
{
    float occluder = t_shadowMapAdv.Sample(s_linearClamp, _posLightTex).r;
    float receiver = exp(-kESMConstant * currentDepth);
    return saturate(occluder * receiver);
}

float CalcVSM(float2 _posLightTex, float currentDepth)
{
    float2 moments = t_shadowMapAdv.Sample(s_linearClamp, _posLightTex).rg;
    float variance = moments.g - moments.r * moments.r;
    float d = currentDepth - moments.r;
    float pMax = variance / (variance + d * d);
    return saturate(pMax);
}

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4 posW = mul(float4(input.posL, 1.f), cb_world);
    output.posH = mul(posW, cb_viewProj);

    // gbuffer texture coord
    output.posClip = output.posH;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float2 posTex = CvtClipToTex(input.posClip);
    float3 baseColor = t_sceneColor.Sample(s_linearClamp, posTex).rgb;
    float3 N = t_sceneNormal.Sample(s_linearClamp, posTex).rgb;
    float depth = t_sceneDepth.Sample(s_linearClamp, posTex).r;
    float3 posW = CvtNdcToWorld(input.posClip.xy / input.posClip.w, depth, cb_invViewProj);

    float dist = distance(cb_lightPos, posW);
    float3 L = (cb_lightPos - posW) / dist;
    float3 V = normalize(cb_eyePos - posW);
    float nDotL = max(dot(N, L), 0.f);

    // distance attenuation
    float blinePhong = CalcBlinePhong(L, V, N, 16);
    float distAtt = CalcDistanceAttenuation(dist, cb_lightFallOffStart, cb_lightFallOffEnd);
    float spotAtt = CalcSpotAttenuation(L, cb_lightDir, cb_lightSpotInnerCos, cb_lightSpotOuterCos);

    float3 color = baseColor * kLightColor * blinePhong * distAtt * spotAtt;
    if (cb_bMakeShadow)
    {
        float4 posLightClip = mul(float4(posW, 1.f), cb_light);
        float2 posLightTex = CvtClipToTex(posLightClip);
        float bias = max(cb_shadowBias * (1.f - nDotL), cb_shadowBias * 0.1f);
        float currentDepth = posLightClip.z / posLightClip.w;
        float shadow = 0.f;

        if (cb_shadowSampling == SHADOW_SAMPLING_HARD)
        {
            shadow = CalcHardShadow(posLightTex, currentDepth, bias);
        }
        else if (cb_shadowSampling == SHADOW_SAMPLING_PCF3)
        {
            shadow = CalcPCF3(posLightTex, currentDepth, bias);
        }
        else if (cb_shadowSampling == SHADOW_SAMPLING_PCF5)
        {
            shadow = CalcPCF5(posLightTex, currentDepth, bias);
        }
        else if (cb_shadowSampling == SHADOW_SAMPLING_PCF7)
        {
            shadow = CalcPCF7(posLightTex, currentDepth, bias);
        }
        else if (cb_shadowSampling == SHADOW_SAMPLING_VSM)
        {
            shadow = CalcVSM(posLightTex, currentDepth);
        }
        else if (cb_shadowSampling == SHADOW_SAMPLING_ESM)
        {
            shadow = CalcESM(posLightTex, currentDepth);
        }

        color *= shadow;
    }
    return float4(color, 1.f);
}