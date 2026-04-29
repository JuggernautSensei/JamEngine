#include "ImageFilterCommon.hlsl"

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 color0 = t_input0.Sample(s_linearWrap, input.texcoord);
    float4 color1 = t_input1.Sample(s_linearWrap, input.texcoord);
    return lerp(color0, color1, params.x);
}