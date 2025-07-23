#include "PostProcessCommon.hlsli"

float4 PSmain(PS_INPUT input) : SV_TARGET
{
    float4 finalColor = ImageFilterInputTexture1.Sample(samplerLinearClamp, input.texCoord);
    return finalColor;
}