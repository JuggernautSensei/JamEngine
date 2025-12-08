#include "ShaderBase.hlsl"

float4 PSMain(FULLSCREEN_PS_INPUT input) : SV_TARGET
{
    float4 color1 = t_input0.Sample(s_linearWrap, input.texCoord0); // Original image
    float4 color2 = t_input1.Sample(s_linearWrap, input.texCoord0); // Bloomed image
    float4 finalColor = lerp(color1, color2, cb_bloomStrength);
    return finalColor;
}

