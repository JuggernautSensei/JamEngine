#include "ShaderBase.hlsl"

float4 PSMain(FULLSCREEN_PS_INPUT _input) : SV_TARGET
{
    // Take 13 samples around the current texel
    float3 a = t_input0.Sample(s_linearClamp, _input.texCoord0 + float2(-2 * cb_texelX, 2 * cb_texelY)).rgb;
    float3 b = t_input0.Sample(s_linearClamp, _input.texCoord0 + float2(0, 2 * cb_texelY)).rgb;
    float3 c = t_input0.Sample(s_linearClamp, _input.texCoord0 + float2(2 * cb_texelX, 2 * cb_texelY)).rgb;

    float3 d = t_input0.Sample(s_linearClamp, _input.texCoord0 + float2(-2 * cb_texelX, 0)).rgb;
    float3 e = t_input0.Sample(s_linearClamp, _input.texCoord0 + float2(0, 0)).rgb;
    float3 f = t_input0.Sample(s_linearClamp, _input.texCoord0 + float2(2 * cb_texelX, 0)).rgb;

    float3 g = t_input0.Sample(s_linearClamp, _input.texCoord0 + float2(-2 * cb_texelX, -2 * cb_texelY)).rgb;
    float3 h = t_input0.Sample(s_linearClamp, _input.texCoord0 + float2(0, -2 * cb_texelY)).rgb;
    float3 i = t_input0.Sample(s_linearClamp, _input.texCoord0 + float2(2 * cb_texelX, -2 * cb_texelY)).rgb;

    float3 j = t_input0.Sample(s_linearClamp, _input.texCoord0 + float2(-cb_texelX, cb_texelY)).rgb;
    float3 k = t_input0.Sample(s_linearClamp, _input.texCoord0 + float2(cb_texelX, cb_texelY)).rgb;
    float3 l = t_input0.Sample(s_linearClamp, _input.texCoord0 + float2(-cb_texelX, -cb_texelY)).rgb;
    float3 m = t_input0.Sample(s_linearClamp, _input.texCoord0 + float2(cb_texelX, -cb_texelY)).rgb;

    // Apply weighted distribution:
    float3 downSample = e * 0.125;
    downSample += (a + c + g + i) * 0.03125;
    downSample += (b + d + f + h) * 0.0625;
    downSample += (j + k + l + m) * 0.125;

    return float4(downSample, 1.f);
}