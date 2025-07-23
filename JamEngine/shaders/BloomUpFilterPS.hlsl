#include "ShaderCommon.hlsl"

float4 PSmain(SCREENSPACE_EFFECT_PS_INPUT input) : SV_TARGET
{
    float x = cb_postProcessBlurRadius;
    float y = cb_postProcessBlurRadius;

    // Take 9 samples around current texel
    float3 a = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(-x, y)).rgb;
    float3 b = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(0, y)).rgb;
    float3 c = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(x, y)).rgb;

    float3 d = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(-x, 0)).rgb;
    float3 e = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(0, 0)).rgb;
    float3 f = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(x, 0)).rgb;

    float3 g = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(-x, -y)).rgb;
    float3 h = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(0, -y)).rgb;
    float3 i = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(x, -y)).rgb;

    // Apply weighted distribution using a 3x3 tent filter
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
    float3 upSample = e * 4.0;
    upSample += (b + d + f + h) * 2.0;
    upSample += (a + c + g + i);
    upSample *= 1.0 / 16.0;

    return float4(upSample, 1.f);
}