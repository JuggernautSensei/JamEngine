#include "ShaderBase.hlsl"

float4 PSMain(FULLSCREEN_PS_INPUT input) : SV_TARGET
{
    float4 color1 = t_input0.Sample(s_linearClamp, input.texCoord0);
    float4 color2 = t_input1.Sample(s_linearClamp, input.texCoord0);
    float4 finalColor = lerp(color2 , color1, cb_combineFactor);
    return finalColor;
}

