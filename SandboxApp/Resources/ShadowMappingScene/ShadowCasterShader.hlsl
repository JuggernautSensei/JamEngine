#include "Common.hlsl"

struct VSInput
{
    float3 posL : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;

#if defined(SHADOW_CAST_VSM) || defined(SHADOW_CAST_ESM) || defined(OMNI_SHADOW_CAST)
    float3 posV : POSITION;
#endif
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    float4 posW = mul(float4(input.posL, 1.f), cb_world);
#if defined(SHADOW_CAST_VSM) || defined(SHADOW_CAST_ESM) || defined(OMNI_SHADOW_CAST)
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
    float d2 = dot(_input.posV, _input.posV);
    return float2(sqrt(d2), d2);
}

#elif defined(SHADOW_CAST_ESM)

float PSMain(PSInput _input) : SV_TARGET
{
    float depth = _input.posV.z;
    return exp(cb_esmExponent * (depth - 1.f));
}

#elif defined (OMNI_SHADOW_CAST)

float PSMain(PSInput _input) : SV_DEPTH
{
    float dist = length(_input.posV);
    return dist / cb_lightFallOffEnd;
}

#else

void PSMain()
{
}

#endif