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

float CalcHardShadow(float3 _sampleDir, float _linearDist, float _bias, float2 _offset = float2(0.f, 0.f))
{
    float3 uvl = CalcTextureArrayLocationFromDir(_sampleDir);
    uvl.xy += _offset;
    float closestDist = t_shadowMapArray.Sample(s_linearClamp, uvl).r;
    return step(_linearDist - _bias, closestDist);
}

float CalcSoftShadow(float3 _sampleDir, float _linearDist, float _bias, float2 _offset = float2(0.f, 0.f))
{
    float3 uvl = CalcTextureArrayLocationFromDir(_sampleDir);
    uvl.xy += _offset;
    return t_shadowMapArray.SampleCmpLevelZero(s_shadowComparison, uvl, _linearDist - _bias).r;
}

float CalcPCF(int _filterSize, float3 _sampleDir, float _linearDist, float _bias)
{
    uint w, h, e;
    t_shadowMapArray.GetDimensions(w, h, e);
    float shadow = 0.f;
    int hSize = _filterSize / 2;
    int size2 = Square(_filterSize);

    [unroll]
    for (int i = -hSize; i <= hSize; ++i)
    {
        for (int j = -hSize; j <= hSize; ++j)
        {
            float2 offset = float2(i / (float) w, j / (float) h);
            if (cb_bUseComparisonSampler)
            {
                shadow += CalcSoftShadow(_sampleDir, _linearDist, _bias, offset);
            }
            else
            {
                shadow += CalcHardShadow(_sampleDir, _linearDist, _bias, offset);
            }
        }
    }

    return shadow / (float) size2;
}

float CalcVSM(float3 _sampleDir, float _currentDepth)
{
    float3 uvl = CalcTextureArrayLocationFromDir(_sampleDir);
    float2 moments = t_shadowMapColorArray.Sample(s_linearClamp, uvl).rg;
    float p = step(_currentDepth, moments.x);
    float minVariance = kVSMMinVariance / Square(cb_lightFallOffEnd);
    float variance = max(moments.y - moments.x * moments.x, minVariance);
    float d = _currentDepth - moments.x;
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

    // final color
    float3 color = baseColor * kLightColor * blinePhong * distAtt;

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

        float3 sampleDir = shadowPosW - cb_lightPos;
        float lightDist = length(sampleDir);
        float linearDist = lightDist / cb_lightFallOffEnd;
        sampleDir /= lightDist;

        float shadow = 0.f;
        if (cb_shadowType == SHADOW_TYPE_HARD)
        {
            shadow = CalcHardShadow(sampleDir, linearDist, cb_shadowBias);
        }
        else if (cb_shadowType == SHADOW_TYPE_PCF3)
        {
            shadow = CalcPCF(3, sampleDir, linearDist, cb_shadowBias);
        }
        else if (cb_shadowType == SHADOW_TYPE_PCF5)
        {
            shadow = CalcPCF(5, sampleDir, linearDist, cb_shadowBias);
        }
        else if (cb_shadowType == SHADOW_TYPE_PCF7)
        {
            shadow = CalcPCF(7, sampleDir, linearDist, cb_shadowBias);
        }
        else if (cb_shadowType == SHADOW_TYPE_VSM)
        {
            shadow = CalcVSM(sampleDir, linearDist);
        }

        color *= shadow;

        if (cb_bRenderLightRegion)
        {
            float3 uvl = CalcTextureArrayLocationFromDir(sampleDir);
            uint l = uvl.z;
            if (l == 0)
            {
                color *= kRegionColor0;
            }
            else if (l == 1)
            {
                color *= kRegionColor1;
            }
            else if (l == 2)
            {
                color *= kRegionColor2;
            }
            else if (l == 3)
            {
                color *= kRegionColor3;
            }
            else if (l == 4)
            {
                color *= kRegionColor4;
            }
            else if (l == 5)
            {
                color *= kRegionColor5;
            }
        }
    }

    return float4(color, 1.f);
}
