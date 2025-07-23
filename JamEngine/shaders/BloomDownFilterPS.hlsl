#include "ShaderCommon.hlsl"

float4 PSmain(SCREENSPACE_EFFECT_PS_INPUT input) : SV_TARGET
{
    uint srcTexWidth;
    uint srcTexHeight;
    uint numOfLevels;
    postProcessInputTexture1.GetDimensions(0, srcTexWidth, srcTexHeight, numOfLevels);
    
    float deltaX = 1.0f / srcTexWidth;
    float deltaY = 1.0f / srcTexHeight;

    // Take 13 samples around the current texel
    float3 a = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(-2 * deltaX, 2 * deltaY)).rgb;
    float3 b = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(0, 2 * deltaY)).rgb;
    float3 c = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(2 * deltaX, 2 * deltaY)).rgb;

    float3 d = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(-2 * deltaX, 0)).rgb;
    float3 e = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(0, 0)).rgb;
    float3 f = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(2 * deltaX, 0)).rgb;

    float3 g = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(-2 * deltaX, -2 * deltaY)).rgb;
    float3 h = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(0, -2 * deltaY)).rgb;
    float3 i = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(2 * deltaX, -2 * deltaY)).rgb;

    float3 j = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(-deltaX, deltaY)).rgb;
    float3 k = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(deltaX, deltaY)).rgb;
    float3 l = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(-deltaX, -deltaY)).rgb;
    float3 m = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord + float2(deltaX, -deltaY)).rgb;

    // Apply weighted distribution:
    float3 downSample = e * 0.125;
    downSample += (a + c + g + i) * 0.03125;
    downSample += (b + d + f + h) * 0.0625;
    downSample += (j + k + l + m) * 0.125;

    return float4(downSample, 1.f);
}