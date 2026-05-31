#include "ImageFilterCommon.hlsl"

float4 PSMain(PSInput _input) : SV_TARGET
{
    uint w, h;
    GetInput0Resolution(w, h);

    float texelX = 1.f / w;
    float texelY = 1.f / h;
    int hLen = cb_length / 2u;
    float4 color = 0.f;

    for (int i = -hLen; i <= hLen; ++i)
    {

#ifdef HORIZONTAL
        float2 stepOffset = {i * texelX, 0.f};
#else
        float2 stepOffset = { 0.f, i * texelY };
#endif

        float2 texcoord = _input.texcoord + stepOffset;
        float4 s = SAMPLE_TEXTURE2D_INPUT0(s_linearClamp, texcoord);
        color += s * GetWeight(i + hLen);
    }

    return color;
}