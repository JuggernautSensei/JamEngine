#include "ShaderBase.hlsl"

float4 PSMain(FULLSCREEN_PS_INPUT _input) : SV_TARGET
{
    float x = cb_bloomBlurRadius;
    float y = cb_bloomBlurRadius;

    // Take 9 samples around current texel
    float3 a = t_input0.Sample(s_linearClamp, _input.texCoord0+ float2(-x, y)).rgb;
    float3 b = t_input0.Sample(s_linearClamp, _input.texCoord0+ float2(0, y)).rgb;
    float3 c = t_input0.Sample(s_linearClamp, _input.texCoord0+ float2(x, y)).rgb;

    float3 d = t_input0.Sample(s_linearClamp, _input.texCoord0+ float2(-x, 0)).rgb;
    float3 e = t_input0.Sample(s_linearClamp, _input.texCoord0+ float2(0, 0)).rgb;
    float3 f = t_input0.Sample(s_linearClamp, _input.texCoord0+ float2(x, 0)).rgb;

    float3 g = t_input0.Sample(s_linearClamp, _input.texCoord0+ float2(-x, -y)).rgb;
    float3 h = t_input0.Sample(s_linearClamp, _input.texCoord0+ float2(0, -y)).rgb;
    float3 i = t_input0.Sample(s_linearClamp, _input.texCoord0+ float2(x, -y)).rgb;

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