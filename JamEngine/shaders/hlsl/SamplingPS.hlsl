#include "ShaderCommon.hlsl"

float4 PSmain(SCREENSPACE_EFFECT_PS_INPUT input) : SV_TARGET
{
    float4 finalColor = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord);
    return finalColor;
}