#include "Common.hlsl"

struct VSInput
{
    float3 posL : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float4 posC : TEXCOORD0; // for gbuffer sampling
};

float CalcHardShadow(float2 _posLightTex, float _currentDepth, float _bias, float2 _offset = float2(0.f, 0.f))
{
    uint w, h;
    t_shadowMap.GetDimensions(w, h);
    float2 uv = _posLightTex + _offset;
    int2 pixel = clamp(int2(uv * float2(w, h)), int2(0, 0), int2(w - 1, h - 1));
    float2 texelUv = (float2(pixel) + 0.5f) / float2(w, h);
    // Compare one stored surface, rather than interpolating unrelated depths.
    float closestDepth = t_shadowMap.Load(int3(pixel, 0)).r;
    float receiverDepth = _currentDepth + dot(texelUv - _posLightTex, CalcReceiverDepthGradient(_posLightTex, _currentDepth));
    return step(receiverDepth - _bias, closestDepth);
}

float CalcSoftShadow(float2 _posLightTex, float _currentDepth, float _bias, float2 _offset = float2(0.f, 0.f))
{
    return t_shadowMap.SampleCmpLevelZero(s_shadowComparison, _posLightTex + _offset, _currentDepth - _bias).r;
}

float CalcPCF(int _filterSize, float2 _posLightTex, float _currentDepth, float _bias)
{
    uint w, h;
    t_shadowMap.GetDimensions(w, h);
    float shadow = 0.f;
    int hSize = _filterSize / 2;
    int size2 = Square(_filterSize);

    float2 depthGradient = CalcReceiverDepthGradient(_posLightTex, _currentDepth);
    float samplingBias = min(dot(abs(depthGradient), 1.f / float2(w, h)), 0.01f);

    [unroll]
    for (int i = -hSize; i <= hSize; ++i)
    {
        [unroll]
        for (int j = -hSize; j <= hSize; ++j)
        {
            float2 offset = float2(i / (float) w, j / (float) h);
            float receiverDepth = _currentDepth + dot(offset, depthGradient);
            if (cb_bUseComparisonSampler)
            {
                shadow += CalcSoftShadow(_posLightTex + offset, receiverDepth, _bias + samplingBias);
            }
            else
            {
                shadow += CalcHardShadow(_posLightTex + offset, receiverDepth, _bias);
            }
        }
    }
    return shadow / (float) size2;
}

float CalcVSM(float2 _posLightTex, float currentDepth)
{
    float2 moments = t_shadowMapColor.Sample(s_linearClamp, _posLightTex).rg;
    float p = step(currentDepth, moments.x);
    float minVariance = kVSMMinVariance / Square(cb_lightFallOffEnd);
    float variance = max(moments.y - moments.x * moments.x, minVariance);
    float d = currentDepth - moments.x;
    float pMax = variance / (variance + d * d);

    // light-bleeding reduction
    return smoothstep(kVSMBleedingReduction, 1.f, max(p, pMax));
}

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4 posW = mul(float4(input.posL, 1.f), cb_world);
    output.posH = mul(posW, cb_viewProj);

    // gbuffer texture coord
    output.posC = output.posH;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float2 posTex = CvtClipToTex(input.posC);
    float3 baseColor = t_sceneColor.Sample(s_linearClamp, posTex).rgb;
    float3 N = t_sceneNormal.Sample(s_linearClamp, posTex).rgb;
    float depth = t_sceneDepth.Sample(s_linearClamp, posTex).r;
    float3 posW = CvtNdcToWorld(input.posC.xy / input.posC.w, depth, cb_invViewProj);

    float dist = distance(cb_lightPos, posW);
    float3 L = (cb_lightPos - posW) / dist;
    float3 V = normalize(cb_eyePos - posW);
    float nDotL = max(dot(N, L), 0.f);

    // distance attenuation
    float blinePhong = CalcBlinePhong(L, V, N, 16);
    float distAtt = CalcDistanceAttenuation(dist, cb_lightFallOffStart, cb_lightFallOffEnd);
    float spotAtt = CalcSpotAttenuation(L, cb_lightDir, cb_lightSpotInnerCos, cb_lightSpotOuterCos);

    // final color
    float3 color = baseColor * kLightColor * blinePhong * distAtt * spotAtt;

    // shadow receiver
    {
        float3 shadowPosW = posW;
        if (cb_shadowType == SHADOW_TYPE_HARD
            || cb_shadowType == SHADOW_TYPE_PCF3
            || cb_shadowType == SHADOW_TYPE_PCF5
            || cb_shadowType == SHADOW_TYPE_PCF7)
        {
            shadowPosW += L * cb_shadowNormalOffset;
        }

        float4 posLightNdc = mul(float4(shadowPosW, 1.f), mul(cb_lightView, cb_lightProj));
        posLightNdc.xyz /= posLightNdc.w;
        float2 posLightTex = CvtNdcToTex(posLightNdc.xy);
        float curDepth = posLightNdc.z;

        float shadow = 0.f;
        if (cb_shadowType == SHADOW_TYPE_HARD)
        {
            shadow = CalcHardShadow(posLightTex, curDepth, cb_shadowBias);
        }
        else if (cb_shadowType == SHADOW_TYPE_PCF3)
        {
            shadow = CalcPCF(3, posLightTex, curDepth, cb_shadowBias);
        }
        else if (cb_shadowType == SHADOW_TYPE_PCF5)
        {
            shadow = CalcPCF(5, posLightTex, curDepth, cb_shadowBias);
        }
        else if (cb_shadowType == SHADOW_TYPE_PCF7)
        {
            shadow = CalcPCF(7, posLightTex, curDepth, cb_shadowBias);
        }
        else if (cb_shadowType == SHADOW_TYPE_VSM)
        {
            curDepth = dist / cb_lightFallOffEnd;
            shadow = CalcVSM(posLightTex, curDepth);
        }

        color *= shadow;

        if (cb_bRenderLightRegion)
        {
            color *= kRegionColor0;
        }
    }


    return float4(color, 1.f);
}
