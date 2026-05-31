#include "ImageFilterCommon.hlsl"

float4 PSMain(PSInput input) : SV_TARGET
{
    return SAMPLE_TEXTURE2D_INPUT0(s_linearClamp, input.texcoord);
}