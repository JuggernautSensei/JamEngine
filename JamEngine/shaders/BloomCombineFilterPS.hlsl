#include "ShaderCommon.hlsl"

float4 PSmain(SCREENSPACE_EFFECT_PS_INPUT input) : SV_TARGET
{
    float4 srcColor = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord);
    float4 backBufferColor = postProcessInputTexture2.Sample(samplerLinearClamp, input.texCoord);
    float4 finalColor = lerp(backBufferColor, srcColor, cb_postProcessCombineStrength);
    return finalColor;
}

