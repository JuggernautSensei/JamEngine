#include "ShadowMappingSceneCommon.hlsl"

struct VSInput
{
    float3 posL : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;

#if defined(SHADOW_CAST_VSM) || defined(SHADOW_CAST_ESM)
    float3 posV : POSITION;
#endif
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    float4 posW = mul(float4(input.posL, 1.f), cb_world);
#if defined(SHADOW_CAST_VSM) || defined(SHADOW_CAST_ESM)
    output.posV = mul(posW, cb_lightView).xyz;
    output.posH = mul(float4(output.posV, 1.f), cb_lightProj);
#else
    output.posH = mul(posW, mul(cb_lightView, cb_lightProj));
#endif 
    return output;
}

#if defined(SHADOW_CAST_VSM)

float2 PSMain(PSInput _input) : SV_TARGET
{
    float depthV = length(_input.posV.xyz);
    float2 moments = 0;
    moments.x = depthV ;
    float dx = ddx(depthV);
    float dy = ddy(depthV);
    moments.y = moments.x * moments.x + 0.25f * (dx * dx + dy * dy);
    return moments;
}

#elif defined(SHADOW_CAST_ESM)

float PSMain(PSInput _input) : SV_TARGET
{
    float depth = _input.posV.z;
    return exp(cb_esmExponent * (depth - 1.f));
}

#else

void PSMain(PSInput _)
{
}

#endif