#include "ImageFilterCommon.hlsl"

float4 PSMain(PSInput input) : SV_TARGET
{
    return t_input0.Sample(s_linearClamp, input.texcoord);
}