#include "ShaderCommon.hlsl"

SCREENSPACE_EFFECT_PS_INPUT VSmain(VS_INPUT_VERTEX2 input)
{
    SCREENSPACE_EFFECT_PS_INPUT output;
    output.positionH = float4(input.positionL, 0.0f, 1.0f);
    output.texCoord = input.uv0;
    return output;
}