#include "PostProcessCommon.hlsli"

PS_INPUT VSmain(VS_2D_INPUT input)
{
    PS_INPUT output;
    output.position = float4(input.positionL, 0.0f, 1.0f);
    output.texCoord = input.texCoord;
    return output;
}