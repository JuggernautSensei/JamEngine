#include "ImageFilterCommon.hlsl"

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 color0 = SAMPLE_TEXTURE2D_INPUT0(s_linearClamp, input.texcoord).rgb;
    float3 color1 = SAMPLE_TEXTURE2D_INPUT1(s_linearClamp, input.texcoord).rgb;
    float3 final = lerp(color0, color1, cb_fparams.x);
    return float4(final, 1.0f);
}