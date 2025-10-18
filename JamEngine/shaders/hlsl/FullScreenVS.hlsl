#include "ShaderBase.hlsl"

FULLSCREEN_PS_INPUT VSMain(VS_INPUT_POS_TEX_2D _input)
{
    FULLSCREEN_PS_INPUT output;
    output.posH      = float4(_input.posL, 0.f, 1.f);
    output.texCoord0 = _input.texCoord;
    return output;
}