#include "ShadowMappingSceneCommon.hlsl"

struct VSInput
{
    float3 posL : POSITION;
};

struct PSInput
{
    float4 posLight : SV_POSITION;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4 posW = mul(float4(input.posL, 1.f), cb_world);
    output.posLight = mul(posW, cb_light);
    return output;
}

#if defined(SHADOW_SAMPLE_VSM)

half2 PSMain(PSInput _input) : SV_TARGET
{
    half2 output;
    output.x = _input.posLight.z;
    output.y = _input.posLight.z * _input.posLight.z;
    return output;
}

#elif defined(SHADOW_SAMPLE_ESM)

float PSMain(PSInput _input) : SV_TARGET
{
    float depth = _input.posLight.z;
    return exp(kESMConstant * depth);
}

#else

void PSMain(PSInput _)
{
}

#endif