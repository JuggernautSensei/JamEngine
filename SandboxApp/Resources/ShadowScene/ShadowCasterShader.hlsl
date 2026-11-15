#include "Common.hlsl"

// SHADOW_MODE_VSM
// SHADOW_MODE_ESM
// LIGHT_OMNI
// LIGHT_DIRECTIONAL

struct VSInput
{
    float3 posL : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;

#if defined(SHADOW_MODE_VSM) || defined(SHADOW_MODE_ESM) || defined(LIGHT_OMNI) || defined(LIGHT_DIRECTIONAL)
    float3 posV : POSITION;
#endif
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    float4 posW = mul(float4(input.posL, 1.f), cb_world);

#if defined(SHADOW_MODE_VSM) || defined(SHADOW_MODE_ESM) || defined(LIGHT_OMNI) || defined(LIGHT_DIRECTIONAL)
    output.posV = mul(posW, cb_lightView).xyz;
    output.posH = mul(float4(output.posV, 1.f), cb_lightProj);
#else
    output.posH = mul(posW, mul(cb_lightView, cb_lightProj));
#endif 

    return output;
}

#if defined(SHADOW_MODE_VSM)

float2 PSMain(PSInput _input) : SV_TARGET
{
#if defined(LIGHT_DIRECTIONAL)
    float depth = _input.posH.z;
#else
    float depth = length(_input.posV) / cb_lightFallOffEnd;
#endif
    // Match the (1, 1) far-plane clear in every light's moment map.
    return float2(depth, depth * depth);
}

#elif defined (LIGHT_OMNI)


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
