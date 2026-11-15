#include "Common.hlsl"

struct VSInput
{
    float2 posL : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float CalcHardShadow(int _index, float2 _posLightTex, float _currentDepth, float _bias, float2 _offset = float2(0.f, 0.f))
{
    uint w, h, layers;
    t_shadowMapArray.GetDimensions(w, h, layers);
    float2 uv = _posLightTex + _offset;
    int2 pixel = clamp(int2(uv * float2(w, h)), int2(0, 0), int2(w - 1, h - 1));
    float2 texelUv = (float2(pixel) + 0.5f) / float2(w, h);
    float closestDepth = t_shadowMapArray.Load(int4(pixel, _index, 0)).r;
    float receiverDepth = _currentDepth + dot(texelUv - _posLightTex, CalcReceiverDepthGradient(_posLightTex, _currentDepth));
    return step(receiverDepth - _bias, closestDepth);
}

float CalcSoftShadow(int _index, float2 _posLightTex, float _currentDepth, float _bias, float2 _offset = float2(0.f, 0.f))
{
    return t_shadowMapArray.SampleCmpLevelZero(s_shadowComparison, float3(_posLightTex + _offset, _index), _currentDepth - _bias).r;
}

float CalcPCF(int _index, uint _filterSize, float2 _posLightTex, float _currentDepth, float _bias)
{
    uint w, h, l;
    t_shadowMapArray.GetDimensions(w, h, l);
    float shadow = 0.f;
    int hSize = (int) _filterSize / 2;
    int size2 = Square(_filterSize);

    // Each tap lies at a different depth on a sloped receiver. Compare against
    // that receiver plane instead of treating the entire kernel as constant Z.
    float2 depthGradient = CalcReceiverDepthGradient(_posLightTex, _currentDepth);
    // The comparison sampler also blends adjacent texels within each tap.
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
                shadow += CalcSoftShadow(_index, _posLightTex + offset, receiverDepth, _bias + samplingBias);
            }
            else
            {
                shadow += CalcHardShadow(_index, _posLightTex + offset, receiverDepth, _bias);
            }
        }
    }
    return shadow / (float) size2;
}

float CalcVSM(int _index, float2 _posLightTex, float currentDepth)
{
    float2 moments = t_shadowMapColorArray.Sample(s_linearClamp, float3(_posLightTex, _index)).rg;
    float p = step(currentDepth, moments.x);
    float depthScale = cb_cascadeLightProj[_index][2][2];
    float minVariance = kVSMMinVariance * Square(depthScale);
    float variance = max(moments.y - moments.x * moments.x, minVariance);
    float d = currentDepth - moments.x;
    float pMax = variance / (variance + d * d);

    // light-bleeding reduction
    return smoothstep(kVSMBleedingReduction, 1.f, max(p, pMax));
}

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posH = float4(input.posL, 0.f, 1.f);
    output.texcoord = CvtNdcToTex(input.posL);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 baseColor = t_sceneColor.Sample(s_linearClamp, input.texcoord).rgb;
    float3 N = t_sceneNormal.Sample(s_linearClamp, input.texcoord).rgb;
    float depth = t_sceneDepth.Sample(s_linearClamp, input.texcoord).r;
    float3 posW = CvtNdcToWorld(CvtTexToNdc(input.texcoord), depth, cb_invViewProj);

    float3 L = normalize(-cb_lightDir);
    float3 V = normalize(cb_eyePos - posW);
    float nDotL = max(dot(N, L), 0.f);
    float blinePhong = CalcBlinePhong(L, V, N, 16);

    // final color
    float3 color = baseColor * kLightColor * blinePhong;

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

        // select layer
        float3 look = normalize(cb_eyeDir);
        float d = dot(shadowPosW - cb_eyePos, look);
        int index = cb_cascadeLevel - 1;
        for (int i = 0; i < cb_cascadeLevel - 1; ++i)
        {
            if (d < cb_cascadeDistances[i])
            {
                index = i;
                break;
            }
        }

        float4 posLightV = mul(float4(shadowPosW, 1.f), cb_cascadeLightView[index]);
        float4 posLightNdc = mul(posLightV, cb_cascadeLightProj[index]);
        posLightNdc.xyz /= posLightNdc.w;
        float2 posLightTex = CvtNdcToTex(posLightNdc.xy);
        float depthLightNdc = posLightNdc.z;

        float shadow = 0.f;
        if (cb_shadowType == SHADOW_TYPE_HARD)
        {
            shadow = CalcHardShadow(index, posLightTex, depthLightNdc, cb_shadowBias);
        }
        else if (cb_shadowType == SHADOW_TYPE_PCF3)
        {
            shadow = CalcPCF(index, 3, posLightTex, depthLightNdc, cb_shadowBias);
        }
        else if (cb_shadowType == SHADOW_TYPE_PCF5)
        {
            shadow = CalcPCF(index, 5, posLightTex, depthLightNdc, cb_shadowBias);
        }
        else if (cb_shadowType == SHADOW_TYPE_PCF7)
        {
            shadow = CalcPCF(index, 7, posLightTex, depthLightNdc, cb_shadowBias);
        }
        else if (cb_shadowType == SHADOW_TYPE_VSM)
        {
            shadow = CalcVSM(index, posLightTex, depthLightNdc);
        }

        color *= shadow;

        
        if (cb_bRenderLightRegion)
        {
            if (index == 0)
            {
                color *= kRegionColor0;
            }
            else if (index == 1)
            {
                color *= kRegionColor1;
            }
            else if (index == 2)
            {
                color *= kRegionColor2;
            }
            else if (index == 3)
            {
                color *= kRegionColor3;
            }
            else
            {
                color *= kRegionColor4;
            }

        }
    }


    return float4(color, 1.f);
}
