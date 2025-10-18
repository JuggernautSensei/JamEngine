#include "ShaderBase.hlsl"

float4 PSMain(FULLSCREEN_PS_INPUT _input) : SV_TARGET
{
    return t_input0.Sample(s_linearClamp, _input.texCoord0);
}