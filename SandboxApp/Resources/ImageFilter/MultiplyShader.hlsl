#include "ImageFilterCommon.hlsl"

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 color0 = SAMPLE_TEXTURE2D_INPUT0(s_linearClamp, input.texcoord);
    float4 color1 = SAMPLE_TEXTURE2D_INPUT1(s_linearClamp, input.texcoord);
    return float4(color0.rgb * color1.rgb, 1.0f);
}